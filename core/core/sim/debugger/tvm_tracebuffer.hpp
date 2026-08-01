#pragma once
#include <functional>
#include <iterator>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include "core/ds/alloc/pagechain.hpp"
#include "core/sim/debugger/tvm_opcodes.hpp"

class RegisterBlaster;

namespace tvm {

// A position within the trace buffer, identifying a specific entry in a specific ring slot.
// Slot indices must be taken % ring size.
// Entry is the index within that slot's indirect buffer.
struct Cursor {
  size_t slot = 0;
  u16 entry = 0;
  auto operator<=>(const Cursor &) const = default;
  bool operator==(const Cursor &) const = default;
};

// Submission queue for TVM programs.
//
// TB does not provide helpers to explicitly serialize opcodes so that it won't need modification with the addition of
// new ops. The class manages the lifetimes of buffers used by a RegisterBlaster, and provides a circular-queue
// abstraction. Submitted programs go to a ring, whose size provides an upper limit of the length of a trace histroy.
//
// Each ring entry can hold ~16k programs, which is limited by the size of an "indirect buffer".
// That indirection buffer exists to make random access in the ring O(1) instead of O(N).
// This is a crucial improvement over the older trace packets system which this class replaces.
// The actual programs themselves can be however long is necessary.
// Program data and code are stored separately per ring entry. There will always be internal fragmentation because
// data/code buffers are not shared between ring entries, which was a deliberate choice to reduce the difficult of
// lifetime mangamenet.
//
// We have a concept of submitter_id, which is used to prevent interleaving of instruction data from different
// submitters Programs are built incrementally in a temporary buffer on a per-submitter-id in 3 parts: a prefix, a body,
// and a postfix. The program is only copied into the ring when end() is called. The body of a program is hashed to
// determine if it has been seen before.
// If so, the program body is replaced with a call.
// The body is copied into a "template" buffer if it has not yet been.
// The template buffer is never freed to avoid dealing with the possibility of use-after-free bugs.
// The prefix and postfix are always inlined and not considered for hashing/replacement.
//
// Since CALLs compile down to ~6 bytes, this should provide footprint reduction for programs which are executed
// frequently. There are only a limited number of meaningful memory access patterns in Pep/10, so I expect a high hit
// rate over time.
//
// The data chain is shared between all submitter_ids.
// Data is submitted immediately rather than being buffer like code.
// This may cause some interleaving of data with concurrent submitters.
// The trace buffer ensures that the resulting programs work correctly, but the interleaved data will likely prevent
// templatization from occuring. This is because the data pointer incrments issued in the body will be different.
//
// For a typical Pep/N trace, I expect programs to be as follows.
//   prefix:  Record current wall time with ASYN, or the wall-time delta with ISYN
//   body:    setmem/setreg paired with DP updated (ACCDP/INCDP/LDP)
//   postfix: termination — always inlined, not hashed (HALT appended by end())
//
// The caller is responsible for "remembering" the state of the registers across calls to emit*.
// The only register this class memoizes is DP, which is required due to the possibility of multiple submitters.
// Each instruction must set all the registers it needs (except DP that is managed by us and IP/SP which are implicit).
// You can't assume register programming survived across end() boundaries due to multiple submitters.
// Within a submitter, you are free to assumer that register programming survives.
// This decision simplifies the TraceBuffer implementation and should increase hit-rates for templatization by reducing
// unnecessary implicit state.
//
// Each submitter tracks its last DP position; the caller computes DP deltas.
class TraceBuffer {
public:
  // Minimum body size (in bytes) to be eligible for template promotion.
  static constexpr u16 PROMOTION_THRESHOLD = 8;
  // Maximum entries per indirect buffer (64KB / sizeof(Buffer::Location)).
  static constexpr u16 MAX_INDIRECT_ENTRIES = pepp::bts::Buffer::SIZE / sizeof(pepp::bts::Buffer::Location);

  TraceBuffer(std::shared_ptr<pepp::bts::BufferManager> mgr, u16 num_submitters, size_t ring_size = 4);
  ~TraceBuffer() noexcept;

  // --- Submission lifecycle ---

  // Begin a new trace for the given submitter.
  // Clears prefix, body, and postfix scratch buffers for this submitter.
  void begin(u16 submitter_id);

  // Finalize the current trace. Appends HALT to the postfix, hashes the body,
  // checks for template promotion, writes an entry to the current ring slot's
  // indirect buffer, and flushes prefix + body (or CALL) + postfix into the
  // code chain. If the indirect buffer is full, advances to the next ring slot.
  pepp::bts::Buffer::Location end(u16 submitter_id);

  // --- Raw emission (call between begin/end) ---

  // Append encoded bytes to the prefix section.
  // Not hashed. Always inlined into the code chain.
  // Typically used to insert timestamps.
  void emit_prefix(u16 submitter_id, bits::span<const u8> encoded);

  // Append encoded bytes to the body section, which will be de-duplicated on calls to end.
  void emit_body(u16 submitter_id, bits::span<const u8> encoded);

  // Append encoded bytes to the postfix section.
  // Not hashed. Always inlined after the body (or CALL). end() appends HALT
  // here automatically; use this to inject instructions before the HALT.
  void emit_postfix(u16 submitter_id, bits::span<const u8> encoded);

  // Append raw data to the current ring slot's shared data chain.
  // Returns the location where the data starts. The caller uses this
  // (along with last_dp()) to construct DP update instructions in the prefix.
  // Immediately updates this submitter's last_dp.
  pepp::bts::Buffer::Location append_data(u16 submitter_id, bits::span<const u8> data);

  // This submitter's last DP position in the shared data chain.
  // Returns {0,0} if this submitter hasn't written data yet.
  pepp::bts::Buffer::Location last_dp(u16 submitter_id) const;

  // --- Backpressure ---

  // Register a callback for when ring occupancy crosses a threshold (0.0 to 1.0).
  // Callbacks fire once per upward crossing; reset when occupancy drops below.
  // Use threshold 1.0 for a "ring full" callback.
  using WatermarkCallback = std::function<void()>;
  void on_watermark(float threshold, WatermarkCallback cb);

  // Mark all slots with index < up_to.slot as consumed.
  // Frees their code and data chains back to the buffer manager.
  void acknowledge(Cursor up_to);

  // --- Cursor / Iteration ---

  // Bidirectional iterator over indirect buffer entries.
  // Dereferencing yields a Buffer::Location pointing to the subroutine for that entry.
  class Iterator {
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = pepp::bts::Buffer::Location;
    using difference_type = std::ptrdiff_t;
    using pointer = const value_type *;
    using reference = value_type;

    Iterator() = default;
    Iterator(const TraceBuffer *tb, Cursor cursor);

    reference operator*() const;
    Iterator &operator++();
    Iterator operator++(int);
    Iterator &operator--();
    Iterator operator--(int);
    bool operator==(const Iterator &other) const;
    bool operator!=(const Iterator &other) const { return !(*this == other); }

  private:
    const TraceBuffer *_tb = nullptr;
    Cursor _cursor{};
  };

  // A range of entries between two cursors [from, to).
  // Supports both forward and reverse iteration.
  struct CursorRange {
    Iterator begin() const { return _begin; }
    Iterator end() const { return _end; }
    std::reverse_iterator<Iterator> rbegin() const { return std::reverse_iterator(_end); }
    std::reverse_iterator<Iterator> rend() const { return std::reverse_iterator(_begin); }

  private:
    friend class TraceBuffer;
    Iterator _begin, _end;
  };

  // Return an iterable range over entries in [from, to).
  CursorRange range(Cursor from, Cursor to) const;

  // Cursor pointing past the last written entry.
  Cursor cursor() const;

  // --- Data chain navigation ---
  // Search all ring nodes' data chains for the successor of the given buffer ID.
  // Returns Buffer::ID{0} if not found.
  pepp::bts::Buffer::ID data_successor(pepp::bts::Buffer::ID id) const;
  // Search all ring nodes' data chains for the predecessor of the given buffer ID.
  // Returns Buffer::ID{0} if not found.
  pepp::bts::Buffer::ID data_predecessor(pepp::bts::Buffer::ID id) const;

  // --- Accessors ---
  size_t ring_size() const { return _ring.size(); }
  u16 submitter_count() const { return static_cast<u16>(_submitters.size()); }
  size_t instruction_count() const { return _total_instructions; }
  // Current ring occupancy: (_head - _tail) / ring_size.
  float ring_occupancy() const;

  // --- Inspect template promotion, mostly used for tests
  // Hash a byte span using the same function as resolve_body.
  static u32 hash(bits::span<const u8> data);
  // Number of promoted templates.
  size_t template_count() const { return _template_map.size(); }
  // Number of hashes seen once (awaiting second occurrence).
  size_t pending_count() const { return _pending_hashes.size(); }
  // True if this hash has been promoted to the template chain.
  bool is_template(u32 h) const { return _template_map.contains(h); }
  // True if this hash has been seen once but not yet promoted.
  bool is_pending(u32 h) const { return _pending_hashes.contains(h); }
  // Hit count for a promoted template. Returns 0 if not promoted.
  u32 template_hits(u32 h) const;
  // Size of a promoted template body (bytes). Returns 0 if not promoted.
  u16 template_size(u32 h) const;

private:
  // --- Ring node ---
  struct Node {
    bool in_use = false;
    // Indirect buffer: array of Buffer::Locations, one per traced instruction.
    pepp::bts::Buffer *indirect = nullptr;
    // Code chain: subroutine bodies, which are prefix + body/CALL + postfix
    std::unique_ptr<pepp::bts::BufferChain> code;
    // Shared data chain: payload bytes for all submitters writing to this slot.
    std::unique_ptr<pepp::bts::BufferChain> data;
    // Number of entries in this slot's indirect buffer.
    u16 count = 0;

    void reset(pepp::bts::BufferManager &mgr);
  };

  // --- Per-submitter recording state ---
  struct SubmitterState {
    std::vector<u8> prefix;
    std::vector<u8> body;
    std::vector<u8> postfix;
    // Last DP this submitter set in the shared data chain.
    pepp::bts::Buffer::Location last_dp{};
    bool active = false;
  };

  // --- Template dedup ---
  struct TemplateEntry {
    pepp::bts::Buffer::Location location;
    u16 size;
    u32 hit_count = 0;
  };

  struct BodyResolution {
    bool is_template;
    // If is_template: location in template chain (target of CALL).
    pepp::bts::Buffer::Location location;
  };
  BodyResolution resolve_body(bits::span<const u8> body);
  pepp::bts::Buffer::Location flush_to_ring(u16 submitter_id, BodyResolution resolution);

  // Advance _head to the next ring slot. Fires watermark callbacks as needed.
  void advance_slot();

  // Write a Buffer::Location into a slot's indirect buffer at position `entry`.
  void write_indirect(Node &node, u16 entry, pepp::bts::Buffer::Location loc);
  // Convert an index in the indirect buffer to an executable buffer location.
  pepp::bts::Buffer::Location read_indirect(const Node &node, u16 entry) const;

  Node &current_node() { return _ring[_head % _ring.size()]; }
  const Node &current_node() const { return _ring[_head % _ring.size()]; }
  const Node &node_at(size_t absolute_slot) const { return _ring[absolute_slot % _ring.size()]; }

  std::shared_ptr<pepp::bts::BufferManager> _mgr;

  std::vector<Node> _ring;
  // _head and _tail may exceed the size of _ring.
  // they must always be taken % _ring.size().
  size_t _head = 0; // Next slot to write
  size_t _tail = 0; // Oldest unconsumed slot

  std::vector<SubmitterState> _submitters;

  // Templates are only freed on TraceBuffer destruction to avoid lifetime management issues.
  std::unique_ptr<pepp::bts::BufferChain> _templates;
  std::unordered_map<u32, TemplateEntry> _template_map;
  // Hashes seen once but not yet promoted. On second occurrence with
  // body.size() >= PROMOTION_THRESHOLD, the body is promoted to _templates.
  std::unordered_set<u32> _pending_hashes;

  // Watermark callbacks are fired when the number of used ring slots crosses a threshold (0.0 to 1.0).
  struct Watermark {
    float threshold;
    WatermarkCallback callback;
    bool fired = false;
  };
  std::vector<Watermark> _watermarks;
  size_t _total_instructions = 0;
};

} // namespace tvm
