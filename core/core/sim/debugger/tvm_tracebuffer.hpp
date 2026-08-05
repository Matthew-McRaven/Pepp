#pragma once
#include <bitset>
#include <functional>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "core/ds/alloc/pagechain.hpp"
#include "core/sim/api/device.hpp"
#include "core/sim/debugger/tvm_machine.hpp"
#include "core/sim/debugger/tvm_opcodes.hpp"


namespace tvm {

class Interpreter;

// Thrown when the ring would advance onto a slot the consumer has never acknowledged. Continuing would destroy trace
// history nobody has read, so the buffer refuses instead of overwriting it.
//
// The recording that triggered this is complete and committed; what failed is the ring's ability to accept *more*
// trace. You can recover by freeing some ring slots. Registering a 1.0 watermark gives you a final chance to make
// space, since watermark callbacks run before the check.
class RingOverflow : public std::runtime_error {
public:
  explicit RingOverflow(size_t slot)
      : std::runtime_error("Trace ring lapped onto unacknowledged slot " + std::to_string(slot)), _slot(slot) {}
  // Absolute index of the slot that could not be reused. Take % ring_size() for the physical slot.
  size_t slot() const { return _slot; }

private:
  size_t _slot;
};

// A position within the trace buffer, identifying a specific entry in a specific ring slot.
// Slot indices must be taken % ring size.
// Entry is the index within that slot's location buffer.
struct Cursor {
  size_t slot = 0;
  u16 entry = 0;
  auto operator<=>(const Cursor &) const = default;
  bool operator==(const Cursor &) const = default;
};

// Trace log of TVM programs.
//
// TB delegates serializing opcodes to trace::Recorder to avoid modification with addition of new ops.
// The class manages the lifetimes of buffers used by a tvm::Interpreter, and provides a circular-queue
// abstraction. Commit()'ed programs go to a ring, whose size provides an upper limit of the length of a trace histroy.
//
// Each ring entry can hold ~8k programs, which is limited by the size of the location buffer.
// The elements of location buffers match the shape of the Interpreter's run_each API.
// This means each location must point to executable code, and each program must terminate with a HALT.
// That location buffer provides an extra level of indirection to make random access in the ring O(1) instead of O(N).
// This is a major improvement over the older trace "packet" systemx.
// The actual programs themselves can be however long is necessary.
// Program data and code are stored separately per ring entry. There will always be internal fragmentation because
// data/code buffers are not shared between ring entries, which was a deliberate choice to reduce the difficultly of
// lifetime mangamenet. As soon as a ring slot is freed, its pages can be released to the BufferManager.
//
//
// Programs are built incrementally in a per-initiator temporary buffer in 3 parts: a prefix, a body,
// and a postfix. The program is only copied into the ring when commit() is called. The body of a program is hashed to
// determine if it has been seen before. If so, the program body is replaced with a call. The body is copied into a
// "template" buffer if it has not yet been. The template buffer is never freed to avoid dealing with the possibility of
// use-after-free bugs. The prefix and postfix are always inlined and not considered for hashing/replacement.
// Grouping recordings by initiating device provide an atomic way to undo a single instruction even when multiple
// initiating devices are in the system.
//
// CALLs compile down to ~6 bytes, which should provide footprint reduction for programs which are executed
// frequently. There are only a limited number of meaningful memory access patterns in Pep/10, so I expect a high hit
// rate over time.
//
// Each initiator gets its own data chain within a ring slot, and data is written immediately rather than being
// buffered like code. Private chains prevent interleaved data from multiple initiators from causing spurious
// templatization failures. Chains are created on first write from an initiator, so the cost is one chain per initiator
// actually recording not one per Device::ID.
//
// For a typical Pep/N trace, I expect programs to be as follows.
//   prefix:  Record current wall time with ASYN, or the wall-time delta with ISYN
//   body:    setmem/setreg paired with DP updated (ACCDP/INCDP/LDP)
//   postfix: termination — always inlined, not hashed (HALT appended by commit())
//
// The only register this class memoizes is DP, which is required because several devices may record concurrently.
// Each program must set all the registers it needs other than DP/SP.
// Register programming does not survive across commit() boundaries due to run_each's RegisterRetention mode.
// This decision simplifies the TraceBuffer implementation and should increase hit-rates for templatization by reducing
// unnecessary implicit state.
class TraceBuffer {
  // Forward-declared so the public Open handle below can name it; defined in the private section.
  struct Recording;

public:
  // Minimum body size (in bytes) to be eligible for template promotion.
  // If a call is 6 bytes, then the body needs to exceed 6 bytes (+ 2 for a ret)
  static constexpr u16 PROMOTION_THRESHOLD = 8;
  // Ceiling on hashes awaiting a second sighting. Bodies that never repeat would otherwise accumulate one entry per
  // program forever, which at tens of millions of instructions is hundreds of MB and a steadily slower lookup.
  static constexpr std::size_t MAX_PENDING_HASHES = 1u << 16;
  // Maximum entries per location buffer (64KB / sizeof(ProgramLocation)).
  static constexpr u16 MAX_LOCATION_ENTRIES = pepp::bts::Buffer::SIZE / sizeof(tvm::ProgramLocation);

  TraceBuffer(std::shared_ptr<pepp::bts::BufferManager> mgr, size_t ring_size = 4);
  ~TraceBuffer() noexcept;

  // --- Recording ---
  // True between begin() and commit() for this initiator. Since UI accesses may trigger traces, we need to be able to
  bool is_recording(Device::ID initiator) const;
  // Look up an in-progress recording. Returns nullptr when the initiator never called begin().
  Recording *find_recording(Device::ID initiator);

  // Begin a new recording for the given initiator, creating its scratch state on first use.
  // Clears the prefix, body, and postfix scratch buffers for this initiator, retaining their capacity.
  void begin(Device::ID initiator);

  // Finalize the current recording. Appends HALT to the postfix, hashes the body,
  // checks for template promotion, writes an entry to the current ring slot's
  // location buffer, and flushes prefix + body (or CALL) + postfix into the
  // code chain. If the location buffer is full, advances to the next ring slot.
  //
  // Throws RingOverflow if advancing would land on a slot that has never been acknowledged. This trace was recorded,
  // but the next one will fail. Free some space before recording again.
  tvm::ProgramLocation commit(Device::ID initiator);

  // Discard an in-progress recording without writing anything to the ring, which occurs when a caller unwinding out of
  // a partially executed instruction due to an exception. Unlike e commit(), a no-op when nothing is recording rather
  // than an assert, since this is typically called from destructors.
  // TODO: we probably need to expose a callback BEFORE the state is cleaned up. Writes have already been applied to
  // registers, and we probably need to undo them.
  void abort(Device::ID initiator);

  // Append encoded bytes to the prefix section.
  // Not hashed. Always inlined into the code chain.
  // Typically used to insert timestamps.
  // Prefer Recording& variant outside of tests.
  void emit_prefix(Device::ID initiator, bits::span<const u8> encoded);
  void emit_prefix(Recording &rec, bits::span<const u8> encoded);

  // Append encoded bytes to the body section, which will be de-duplicated on calls to commit.
  // Prefer Recording& variant outside of tests.
  void emit_body(Device::ID initiator, bits::span<const u8> encoded);
  void emit_body(Recording &rec, bits::span<const u8> encoded);

  // Append encoded bytes to the postfix section.
  // Not hashed. Always inlined after the body (or CALL). commit() appends HALT
  // here automatically; use this to inject instructions before the HALT.
  // Prefer Recording& variant outside of tests.
  void emit_postfix(Device::ID initiator, bits::span<const u8> encoded);
  void emit_postfix(Recording &rec, bits::span<const u8> encoded);

  // Append raw data to this initiator's data chain in the current ring slot.
  // Returns the location where the data starts. The caller uses this
  // (along with last_dp()) to construct DP update instructions in the prefix.
  // Immediately updates this initiator's last_dp.
  pepp::bts::Buffer::Location append_data(Device::ID initiator, bits::span<const u8> data);

  // Unitialized data and a writable view of it.
  struct DataSlot {
    pepp::bts::Buffer::Location loc;
    bits::span<u8> bytes;
  };
  // Reserves a contiguous chunk of the data chain without initializing the memory, and return that memory as a span of
  // bytes. Allows us to avoid an extra data copy in some places. Also updates the initiator's last_dp.
  DataSlot append_data_uninitialized(Device::ID initiator, std::size_t len);

  // This initiator's last DP position in its own data chain.
  // Returns {0,0} if this initiator has never written data.
  pepp::bts::Buffer::Location last_dp(Device::ID initiator) const;

  DataSlot append_data_uninitialized(Recording &rec, std::size_t len);

  // Where the emitted program has pointed DP so far within this recording, and the size it set. `set` is false at the
  // start of every recording, because a program may not assume register state survived a commit() boundary -- replay
  // can begin at any cursor. An emitter uses this to decide between an absolute DP load and a cheaper delta.
  //
  // Tracked per recording rather than per emitter because several devices contribute to one CPU instruction's record,
  // and they share the one data pointer.
  struct DpAnchor {
    bool set = false;
    pepp::bts::Buffer::Location at{};
    // DS the program set, i.e. the payload size.
    u16 size = 0;
    // Bytes this payload actually reserved, which is larger than `size` when the record carries something ahead of
    // the payload. SETMEMDX reserves an extra 4 bytes for a target address.
    // If stride != size, when you update the DP you MUST use INCDP and advance by stride rather than ACCCDP (with
    // size). SETMEMDX+ACCDP would leave DP 4 bytes behind the new payload.
    u16 stride = 0;
  };
  DpAnchor dp_anchor(Recording &rec) const;
  void set_dp_anchor(Recording &rec, pepp::bts::Buffer::Location at, u16 size, u16 stride);

  void trace(Device::ID device, bool enabled = true) { _traced[device.value] = enabled; }
  bool traced(Device::ID device) const { return _traced[device.value]; }

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

  // Bidirectional iterator over location buffer entries.
  // Dereferencing yields the ProgramLocation for that entry, which gives us the starting code and data addresses.
  class Iterator {
  public:
    // Can't be iterator_category=bidirectional_iterator_tag, because dereferencing yields a value, not a reference.
    // For legacy algorithms, this can only be input_iterator_tag. But for C++20 ranges, we can advertise the full
    // bi-directional concept.
    using iterator_concept = std::bidirectional_iterator_tag;
    using iterator_category = std::input_iterator_tag;
    using value_type = tvm::ProgramLocation;
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
  // Number of distinct initiators that have ever recorded. Entries persist after commit() so their scratch capacity
  // is reused, so this counts devices seen, not devices currently recording.
  size_t recording_count() const { return _recordings.size(); }
  size_t instruction_count() const { return _footprint.programs; }
  // Current ring occupancy: (_head - _tail) / ring_size.
  float ring_occupancy() const;

  // --- Footprint accounting ---
  // A class containing performance counters (and metrics derived from them) to analyze the effectiveness of our
  // promotion scheme. Prefer counters over a promotion_enable flag, which would require 2 runs of the same program to
  // compare effectiveness. This counters are monotonically non-decreasing, and must survive acknowledge().
  struct Footprint {
    // Bytes appended to ring code chains -- prefix + (body or CALL) + postfix -- summed over every committed program.
    std::size_t code = 0;
    // Code size if ever body was inlined instead of using a CALL to a template.
    // The difference `code_if_inlined - code` is the # of bytes saved by templating.
    std::size_t code_if_inlined = 0;
    // Bytes appended to the template chain, which is the promoted body plus ret.
    // This is an unreclaimable cost, and must be accounted for in the compression ration.
    std::size_t templates = 0;
    // Bytes appended to data chains, which cannot be compressed. Ideally, this would be the largest % of the total
    // bytes, which means our compression works.
    std::size_t data = 0;
    // Programs committed.
    std::size_t programs = 0;

    // Bytes written to location buffers. Worth accounting for at all because location buffers are plain Buffers
    // rather than chains, so nothing else here sees them, and omitting them understates the total by 8 per program.
    std::size_t locations() const { return programs * sizeof(tvm::ProgramLocation); }

    // Retained bytes with promotion on, and what the same trace would have cost with it off. Location bytes sit on
    // both sides: promotion does not change how many programs there are.
    std::size_t total() const { return code + templates + data + locations(); }
    std::size_t total_if_inlined() const { return code_if_inlined + data + locations(); }
    // The number worth quoting against the old packet format. 0 when nothing has been committed.
    double bytes_per_program() const { return programs ? (double)total() / (double)programs : 0.0; }
    double bytes_per_program_if_inlined() const {
      return programs ? (double)total_if_inlined() / (double)programs : 0.0;
    }
    // >1 means promotion is winning. Counts bytes written, not buffers reserved -- see buffer_footprint() for that.
    double compression_ratio() const { return total() ? (double)total_if_inlined() / (double)total() : 0.0; }
  };
  // A snapshot, by value: callers routinely take one before a run and another after, and compare them.
  Footprint footprint() const;

  // Reset all footprint /counters/ to 0 while retaining all other state inside the class.
  // Cost comparisons involving templates will be incorrect because exisitng templates' cost will no longer accounted
  // for. A full reset to the TraceBuffer must also invoke this method.
  void reset_footprint();

  // Bytes of buffer this TraceBuffer's chains and location buffers currently hold. Unlike Footprint this counts what
  // the allocator reserved rather than what was written, so it includes the unused tail of every partially-filled
  // buffer -- the cost per-initiator data chains trade away to keep bodies templatizable.
  std::size_t buffer_footprint() const;

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
    // Location buffer: array of Buffer::Locations, one per traced instruction.
    pepp::bts::Buffer *locations = nullptr;
    // Code chain: subroutine bodies, which are prefix + body/CALL + postfix
    std::unique_ptr<pepp::bts::BufferChain> code;
    // One data chain per initiator writing into this slot, so that concurrent recorders cannot fragment each other's
    // payloads. Created on first write and then kept across reset() -- a cleared chain owns no buffers, so a retained
    // entry costs one map node and saves rebuilding the chain for an initiator that records here again.
    std::unordered_map<Device::ID, std::unique_ptr<pepp::bts::BufferChain>, pepp::handle_hash<Device::ID>> data;
    // Number of entries in this slot's location buffer.
    u16 count = 0;

    void reset(pepp::bts::BufferManager &mgr);
  };

  // --- Per-initiator recording state ---
  // Do not reduce capcity between iterations. After some # of instructions, I expect we'll reach a steady state and
  // these buffers will stop growing.
  struct Recording {
    std::vector<u8> prefix;
    std::vector<u8> body;
    std::vector<u8> postfix;
    // Which initiator this belongs to, which is needed to find the right data chain.
    Device::ID id{};
    // Last DP this initiator set in its own data chain.
    pepp::bts::Buffer::Location last_dp{};
    bool active = false;
    // See DpAnchor. Reset by begin().
    DpAnchor dp{};
    // Where this record's *first* data payload byte landed, which is what commit() stores in the location buffer so the
    // driver can point DP at it before entering the program. Distinct from DpAnchor::at, which tracks the most recent
    // payload. Null when the record wrote nothing.
    pepp::bts::Buffer::Location data_start{};
    // Memoize the result of data_chain (in addition to its slot #) to avoid repeated map lookups in the hot recording
    // path. This keeps the write path to a size_t compare instead of a map lookup and the slot indices are never
    // invalidated. A ring that has moved on forces a re-resolve.
    pepp::bts::BufferChain *chain = nullptr;
    size_t chain_slot = 0;
  };

  // --- Template dedup ---
  struct TemplateEntry {
    // Where a CALL to this template should aim.
    pepp::bts::Buffer::Location location;
    u32 hit_count = 0;
    // The bytes of the promoted body. On a hash hit, we want to compare the actual bytes to avoid collisions.
    // This span pre-resolves location back to its buffer, avoiding a walk of the template chain on hit.
    // While our size is really only a u16, it gets promoted to size_t on account of being a span. Always downcast size
    // to 16 bits before use.
    //
    // The pointer is safe to hold as long as the template chain is not cleared, and as long as a Buffer's data is not
    // moved out of.
    bits::span<const u8> body{};
  };

  struct BodyResolution {
    bool is_template;
    // If is_template: location in template chain (target of CALL).
    pepp::bts::Buffer::Location location;
  };
  BodyResolution resolve_body(bits::span<const u8> body);
  // True when the template recorded in `entry` holds exactly `body`. resolve_body keys templates on a truncated
  // 32-bit hash, so a map hit alone does not prove the bodies match; this is what makes a collision safe.
  static bool template_matches(const TemplateEntry &entry, bits::span<const u8> body);
  tvm::ProgramLocation flush_to_ring(Recording &rec, BodyResolution resolution);

  // This recording's data chain in the ringbuffer's head slot, creating the chain on first use.
  pepp::bts::BufferChain &data_chain(Recording &rec);

  // Advance _head to the next ring slot. Fires watermark callbacks as needed.
  void advance_slot();

  // Write a ProgramLocation into a slot's location buffer at position `entry`.
  void write_location(Node &node, u16 entry, tvm::ProgramLocation program);
  // Read back the ProgramLocation stored at an index in the location buffer.
  tvm::ProgramLocation read_location(const Node &node, u16 entry) const;

  Node &current_node() { return _ring[_head % _ring.size()]; }
  const Node &current_node() const { return _ring[_head % _ring.size()]; }
  const Node &node_at(size_t absolute_slot) const { return _ring[absolute_slot % _ring.size()]; }

  std::shared_ptr<pepp::bts::BufferManager> _mgr;

  std::vector<Node> _ring;
  // _head and _tail may exceed the size of _ring.
  // they must always be taken % _ring.size().
  size_t _head = 0; // Next slot to write
  size_t _tail = 0; // Oldest unconsumed slot

  // Sparse on purpose: only devices that actually initiate accesses ever appear, and which those are is not known
  // until one records. Sizing this to the Device::ID space would allocate hundreds of idle std::vectors to serve the
  // one or two CPUs that are real initiators. Entries are created on first begin() and then kept, so a device's
  // scratch buffers keep their capacity across programs instead of reallocating on every instruction.
  std::unordered_map<Device::ID, Recording, pepp::handle_hash<Device::ID>> _recordings;

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
  // Indexed by Device::ID, whose underlying type is u8. 32 bytes, and a lookup is a word load plus a bit test.
  std::bitset<256> _traced;

  // Accumulated performance counters.
  Footprint _footprint;
};

} // namespace tvm
