#pragma once
#include <array>
#include <bitset>
#include <functional>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include "core/ds/alloc/pagechain.hpp"
#include "core/sim/api/device.hpp"
#include "core/sim/debugger/tvm_machine.hpp"
#include "core/sim/debugger/tvm_opcodes.hpp"

class Traceable;

namespace tvm {

class Interpreter;

// Thrown by begin() if it would cause _head to point to an unacknowledged slot. Continuing would
// destroy trace history nobody has read and the buffer refuses this operation. No data is lost, but the TraceBuffer has
// lost the ability to accept new data. You recover by acknowledging some slots. Registering a 1.0 watermark gives you a
// final chance to make space before the ring overflows.
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
struct Cursor {
  size_t slot = 0; // must be taken % ring size.
  u16 ordinal = 0; // which entry within that slot's location buffer.
  auto operator<=>(const Cursor &) const = default;
  bool operator==(const Cursor &) const = default;
};

// Unitialized data and a writable view of it.
struct DataSlot {
  pepp::bts::Buffer::Location loc;
  bits::span<u8> bytes;
};

struct DpAnchor {
  // False if no program has written data yet
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

struct Recording {
  // Buffers to hold uncommitted code, which should not be reduced in size between iterations. After some # of
  // instructions, I expect we'll reach a steady state and these buffers will stop growing and we can avoid dynamic
  // memory allocation in the long run.
  std::vector<u8> prefix, body, postfix;
  // Which initiator this belongs to, which is needed to find the right data chain.
  Device::ID id{};
  bool active = false;
  // See DpAnchor. Reset by begin().
  DpAnchor dp{};
  // Where this record's first data payload byte landed, which is what commit() stores in the location buffer so the
  // driver can point DP at it before entering the program. Distinct from DpAnchor::at, which tracks the most recent
  // payload. Null when the record wrote nothing.
  pepp::bts::Buffer::Location data_start{};
  // The ring slot and location-buffer ordinal this recording claimed at begin().
  std::size_t slot = 0;
  u16 ordinal = 0;
  // Memoize the result of data_chain to avoid a map lookup on every traced write. Resolved once per recording now
  // that the slot cannot move underneath it; begin() clears it.
  pepp::bts::BufferChain *chain = nullptr;
};

// TB delegates serializing opcodes to trace::Recorder to avoid modification with addition of new ops.
// The class manages the lifetimes of buffers used by a tvm::Interpreter, and provides a circular-queue
// abstraction. Commit()'ed programs go to a ring, whose size provides an upper limit of the length of a trace histroy.
//
// Each ring entry can hold 8-30k programs, which is limited by the size of the location buffer.
// The elements of location buffers match the shape of the Interpreter's run_each API.
// This means each location must point to executable code, and each program must terminate with a HALT.
// That location buffer provides an extra level of indirection to make random access in the ring O(1) instead of O(N).
// This is a major improvement over the older trace "packet" systemx.
// The actual programs themselves can be however long is necessary.
// Program data and code are stored separately per ring entry. There will always be internal fragmentation because
// data/code buffers are not shared between ring entries, which was a deliberate choice to reduce the difficultly of
// lifetime mangamenet. As soon as a ring slot is freed, its pages can be released to the BufferManager.
//
// Programs are built incrementally in a per-initiator temporary buffer in 3 parts: a prefix, a body,
// and a postfix. begin() claims the ring slot and the ordinal the program will occupy but writes nothing for it;
// commit() copies the bytes into the ring and only then writes the location entry, because an entry is encoded
// against the one in front of it. At most one recording is open at a time, so claim order is commit order, and a
// recording that outlives a slot advance still writes into the slot it began in. The body of a program is hashed to
// determine if it has been seen before. If so, the program body is replaced with a call. The body is copied into a
// stencil buffer if it has not yet been. The stencil buffer is never freed to avoid dealing with the possibility of
// use-after-free bugs. The prefix and postfix are always inlined and not considered for hashing/replacement.
// Grouping recordings by initiating device provide an atomic way to undo a single instruction even when multiple
// initiating devices are in the system.
//
// CALLs compile down to 4-6 bytes, which should provide footprint reduction for programs which are executed
// frequently. There are only a limited number of meaningful memory access patterns in Pep/10, so I expect a high
// stencil hit rate over time.
//
// Each initiator gets its own data chain within a ring slot, and data is written immediately rather than being
// buffered like code. Private chains prevent interleaved data from multiple initiators from causing spurious
// stencil dedup failures. Chains are created on first write from an initiator, so the cost is one chain per initiator
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
// This decision simplifies the TraceBuffer implementation and should increase stencil hit-rates by reducing
// unnecessary implicit state.
class TraceBuffer {
public:
  // Ceiling on hashes awaiting a second sighting. Bodies that never repeat would otherwise accumulate one entry per
  // program forever, which at tens of millions of instructions is hundreds of MB and a steadily slower lookup.
  static constexpr std::size_t MAX_PENDING_HASHES = 1u << 16;
  // Maximum number of bytes occupied by a single location buffer entry.
  static constexpr u8 MAX_LOCATION_ENTRY_BYTES = 10;
  // Compute the offset element-wise of value-anchor, and encode each element using SLEB128. Returns the number of
  // bytes written. Throws when out is shorter than MAX_LOCATION_ENTRY_BYTES.
  static u8 encode_location(bits::span<u8> out, tvm::ProgramLocation anchor, tvm::ProgramLocation value);
  // Inverse of encode_location. Given an anchor and a pair of SLEB128-encoding integers, recompute the original value.
  // The number of consumed bytes is written to size. If size is 0, there were insufficient bytes in the input buffer.
  static tvm::ProgramLocation decode_location(bits::span<const u8> in, tvm::ProgramLocation anchor, u8 &size);

  // ring_size must be a power of two and throws if it is not.
  TraceBuffer(std::shared_ptr<pepp::bts::BufferManager> mgr, size_t ring_size = 4);
  ~TraceBuffer() noexcept;

  // Helpers from the system to convert Device::IDs to instances of classes that are Traceable.
  // Return nullptr if the device ID does not exist or is not traceable.
  using TraceableFinder = std::function<Traceable *(Device::ID)>;
  void set_traceable_finder(TraceableFinder finder);
  Traceable *find_traceable(Device::ID initiator) const;

  // --- Recording ---
  // True between begin() and commit() for this initiator.
  bool is_recording(Device::ID initiator) const;
  // The open recording, when it belongs to this initiator. Returns nullptr when nothing is recording, or when what
  // is recording belongs to someone else.
  Recording *find_recording(Device::ID initiator);

  // Begin a new recording for the given initiator, creating its scratch state on first use and retaining scratch space
  // across usages to reduce dynamic allocation frequency. This method claims a ring slot and an ordinal in its
  // location buffer, held until either commit() or abort(). At most one recording is open at a time, so claim order
  // is commit order; abort() gives the ordinal back, and until commit() writes it, it reads as a program which
  // immediately halts.
  //
  // Throws RingOverflow if this call would destroy data (i.e., this advances to a new slot, and that slot is in use).
  // This is the only place that refusal is raised, so an instruction that gets past begin() is guaranteed somewhere to
  // commit into. Nothing is opened when it throws.
  void begin(Device::ID initiator);

  // Finalize the current recording. Appends HALT to the postfix, hashes the body, checks for stencil promotion,
  // flushes prefix + (body or CALL) + postfix into the code chain, and writes its location entry. If this
  // ring slot is full and no other recordings are open, advances to the next slot, which can fire watermark callbacks.
  //
  // Can throw if the buffer manager runs out of buffers.
  tvm::ProgramLocation commit(Device::ID initiator);

  // Discard an in-progress recording without writing anything to the ring, which occurs when a caller unwinding out of
  // a partially executed instruction due to an exception. The ordinal it claimed is given back, so an aborted
  // instruction leaves no entry behind; any payload it wrote is stranded in the data chain until the slot is
  // reclaimed. Unlike e commit(), a no-op when nothing is recording rather
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
  // Not hashed. Always inlined after the body (or CALL). commit() ends every program with a HALT after the postfix;
  // use this to inject instructions before it.
  // Prefer Recording& variant outside of tests.
  void emit_postfix(Device::ID initiator, bits::span<const u8> encoded);
  void emit_postfix(Recording &rec, bits::span<const u8> encoded);

  // Append raw data to this initiator's data chain in the ring slot this recording claimed at begin().
  // Returns the location where the data starts. The caller pairs that with dp_anchor() to work out how to step DP
  // onto it, and records the result with set_dp_anchor().
  pepp::bts::Buffer::Location append_data(Device::ID initiator, bits::span<const u8> data);

  // Reserves a contiguous chunk of the data chain without initializing the memory, and return that memory as a span of
  // bytes. Allows us to avoid an extra data copy in some places. Reports the location the same way append_data does.
  DataSlot append_data_uninitialized(Device::ID initiator, std::size_t len);

  DataSlot append_data_uninitialized(Recording &rec, std::size_t len);

  // Where the emitted program has pointed DP so far within this recording, and the size it set. `set` is false at the
  // start of every recording, because a program may not assume register state survived a commit() boundary -- replay
  // can begin at any cursor. An emitter uses this to decide between an absolute DP load and a cheaper delta.
  //
  // Tracked per recording rather than per emitter because several devices contribute to one CPU instruction's record,
  // and they share the one data pointer.
  DpAnchor dp_anchor(Recording &rec) const;
  void set_dp_anchor(Recording &rec, pepp::bts::Buffer::Location at, u16 size, u16 stride);

  // Sets the bit and notifies the device of the update via Traceable::on_trace_changed.
  void trace(Device::ID device, bool enabled = true);
  bool traced(Device::ID device) const { return _traced[device.value]; }

  // When false, we prefer packet formats which encode offsets in the instruction,
  // When true, we prefer packet formats which encode offsets in the datastream.
  // If you have a consistent access pattern to offsets (register banks), set to false. All other cases should be true.
  // Moving the address into the payload makes updating the offset more expensive but provides more opportunities for
  // de-duplication. Not all operations support both formats, in which case this recorder will choose whichever is
  // available
  void set_address_in_payload(Device::ID device, bool enabled) { _address_in_payload[device.value] = enabled; }
  bool address_in_payload(Device::ID device) const { return _address_in_payload[device.value]; }

  // Address-space size at or below which System::bind_recorders will set_address_in_payload(true) for that device.
  // Register banks, CSRs are small and have predictable access patterns. Main memory is usually big and less
  // predictable. This is an atbitrary threshold which tries to select the right behavior by default. Systems with
  // device-specific knowledge aree free to ignore this suggestion.
  static constexpr std::size_t DEFAULT_NARROW_TARGET_BYTES = 256;

  // --- Backpressure ---

  // Register a callback for when ring occupancy crosses a threshold (0.0 to 1.0).
  // Callbacks fire once per upward crossing; reset when occupancy drops below.
  // Use threshold 1.0 for a "ring full" callback.
  using WatermarkCallback = std::function<void()>;
  void on_watermark(float threshold, WatermarkCallback cb);

  // Mark all slots with index < up_to.slot as consumed.
  // Frees their code and data chains back to the buffer manager.
  void acknowledge(Cursor up_to);

  // Drop all recordings, stencils, ring entries, etc and returns this class to it's post-construction state. Unlike
  // acknowledge, this will force-clear slots currently being written by aborting in-progress writes.
  // Configuration parameters survive, such as the set of traced devices and watermarks.
  void clear();

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

  // Cursor past the last *reserved* entry, in-flight recordings included. An instruction that has begun but not
  // committed is inside this range and dereferences to a program that immediately halts, so a consumer that races a
  // recording replays a no-op rather than reading a half-written entry.
  //
  // The content of such an entry changes when that recording commits, so a range taken against this end is not
  // stable. Iterate it twice across a commit and the same position yields a halt and then the real program. Use it
  // when you want everything the buffer knows about, including the instruction currently executing.
  Cursor cursor() const;

  // Equal to cursor() when nothing is recording, otherwise equal to the open recording's entry.
  Cursor committed_cursor() const;

  // --- Data chain navigation ---
  // Search all ring nodes' data chains for the successor of the given buffer ID.
  // Returns Buffer::ID{0} if not found.
  pepp::bts::Buffer::ID data_successor(pepp::bts::Buffer::ID id) const;
  // Search all ring nodes' data chains for the predecessor of the given buffer ID.
  // Returns Buffer::ID{0} if not found.
  pepp::bts::Buffer::ID data_predecessor(pepp::bts::Buffer::ID id) const;

  // --- Accessors ---
  std::size_t ring_size() const { return _ring.size(); }
  // True when the head slot cannot take another entry, so the next begin() advances the ring.
  bool head_slot_full() const { return slot_full(current_node()); }
  // A HALT that lives as long as the buffer to which CALLHALT and STCALLHALT return.
  pepp::bts::Buffer::Location halt_location() const { return _tombstone.code; }
  // Where STCALL's index points. Buffer::ID{0} for an invalid index.
  pepp::bts::Buffer::Location stencil_location(u16 index) const {
    return index < _stencil_locations.size() ? _stencil_locations[index] : pepp::bts::Buffer::Location{};
  }
  // Number of distinct initiators that have ever recorded. Entries persist after commit() so their scratch capacity
  // is reused, so this counts devices seen, not devices currently recording.
  std::size_t recording_count() const { return _recordings.size(); }
  std::size_t instruction_count() const { return _footprint.programs; }
  // Current ring occupancy: (_head - _tail) / ring_size.
  float ring_occupancy() const;

  // --- Footprint accounting ---
  // A class containing performance counters (and metrics derived from them) to analyze the effectiveness of our
  // promotion scheme. Prefer counters over a promotion_enable flag, which would require 2 runs of the same program to
  // compare effectiveness. This counters are monotonically non-decreasing, and must survive acknowledge().
  struct Footprint {
    // Bytes appended to ring code chains -- prefix + (body or CALL) + postfix -- summed over every committed program.
    std::size_t code = 0;
    // Code size if ever body was inlined instead of using a CALL to a stencil.
    // The difference `code_if_inlined - code` is the # of bytes saved by stencil dedup.
    std::size_t code_if_inlined = 0;
    // Bytes appended to the stencil chain, which is the promoted body plus ret.
    // This is an unreclaimable cost, and must be accounted for in the compression ration.
    std::size_t stencils = 0;
    // Bytes appended to data chains, which cannot be compressed. Ideally, this would be the largest % of the total
    // bytes, which means our compression works.
    std::size_t data = 0;
    // Programs committed.
    std::size_t programs = 0;

    // Total number of bytes written to the location buffer
    std::size_t location_bytes = 0;
    // Additional out-of-buffer bytes required for bookkeeping. Only charge for the used bytes rather than the static
    // allocations.
    std::size_t location_aux_bytes = 0;
    std::size_t locations() const { return location_bytes + location_aux_bytes; }

    // Retained bytes with promotion on, and what the same trace would have cost with it off. Location bytes sit on
    // both sides: promotion does not change how many programs there are.
    std::size_t total() const { return code + stencils + data + locations(); }
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
  // One string describing the buffer's footprint with a prefixed label.
  std::string describe(std::string_view label, const Footprint &f) const;
  std::string describe(std::string_view label) const { return describe(label, footprint()); }

  // Reset all footprint /counters/ to 0 while retaining all other state inside the class.
  // Cost comparisons involving stencils will be incorrect because existing stencils' cost will no longer accounted
  // for. A full reset to the TraceBuffer must also invoke this method.
  void reset_footprint();

  // Bytes of buffer this TraceBuffer's chains and location buffers currently hold. Unlike Footprint this counts what
  // the allocator reserved rather than what was written, so it includes the unused tail of every partially-filled
  // buffer -- the cost per-initiator data chains trade away to keep bodies dedup-eligible.
  std::size_t buffer_footprint() const;

  // --- Inspect stencil promotion, mostly used for tests
  // Hash a byte span using the same function as resolve_body.
  static u32 hash(bits::span<const u8> data);
  // Number of promoted stencils.
  size_t stencil_count() const { return _stencil_map.size(); }
  // Number of hashes seen once (awaiting second occurrence).
  size_t pending_count() const { return _pending_hashes.size(); }
  // True if this hash has been promoted to the stencil chain.
  bool is_stencil(u32 h) const { return _stencil_map.contains(h); }
  // True if this hash has been seen once but not yet promoted.
  bool is_pending(u32 h) const { return _pending_hashes.contains(h); }
  // Hit count for a promoted stencil. Returns 0 if not promoted.
  u32 stencil_hits(u32 h) const;
  // Size of a promoted stencil body (bytes). Returns 0 if not promoted.
  u16 stencil_size(u32 h) const;

private:
  // Minimum body size (in bytes) to be eligible for stencil promotion. Sized to CALL+RET (6+2) bytes.
  static constexpr u16 PROMOTION_THRESHOLD = 8;
  // Maximum number of stencils that can be stored, limited by STCALL's u16 index.
  static constexpr std::size_t MAX_STENCILS = std::size_t{1} << 16;

  // Uncompressed ProgramLocations are 8-bytes each, and are a limiting factor on how many programs we can fit per ring
  // slot. By compressing ProgramLocations into a variable-width delta from the previous entry, we can reduce the number
  // of bytes needed to store a ProgramLocation. However, we then lose random-access to the location buffer.
  // Every LOCATION_GROUP_STRIDE bytes we inserte a full, uncompressed ProgramLocation, which we cann an anchor.
  // A group is all of the program locations from an anchor up to (but not including) the next anchor. All locations
  // within a group besides the anchor are encoded as a delta from the previous location.
  // These parameters control the distance between anchors. After a search of all power-of-2 anchors, I determined that
  // a stride of 8 produced good savings while keeping random access O(small constant).
  static constexpr u16 LOCATION_GROUP_STRIDE = 8 * sizeof(tvm::ProgramLocation);
  // Groups per location buffer.
  static constexpr std::size_t LOCATION_GROUPS = pepp::bts::Buffer::SIZE / LOCATION_GROUP_STRIDE;
  // Maximum entries per location buffer limited by index size (u16).
  static constexpr u16 MAX_LOCATION_ENTRIES = std::numeric_limits<u16>::max();
  static_assert(pepp::bts::Buffer::SIZE % LOCATION_GROUP_STRIDE == 0, "groups must divide a buffer evenly");
  static_assert(LOCATION_GROUP_STRIDE >= sizeof(tvm::ProgramLocation) + MAX_LOCATION_ENTRY_BYTES,
                "a group must fit at least two entries");

  // --- Ring node ---
  // Absolute slot index meaning "this node holds nothing". _head never approaches it.
  static constexpr std::size_t NO_SLOT = SIZE_MAX;

  struct Node {
    // Which absolute ring slot currently occupies this node, or NO_SLOT when it holds nothing.
    //
    // A Cursor names an *absolute* slot, but a node is found by absolute_slot % ring_size -- so slot 1 and slot 5 of a
    // four-slot ring are the same node. Without this stamp an iterator into a slot the ring had since reused would
    // silently read the newer slot's entries and hand back a real-looking program from the wrong point in history.
    // Comparing against it turns that into a null location, which is easier to gaurd against.
    //
    // It doubles as the "this slot holds unacknowledged trace" flag begin() refuses to write over.
    std::size_t slot = NO_SLOT;
    // Location buffer: array of Buffer::Locations, one per traced instruction.
    pepp::bts::Buffer *locations = nullptr;
    // Code chain: subroutine bodies, which are prefix + body/CALL + postfix
    std::unique_ptr<pepp::bts::BufferChain> code;
    // One data chain per initiator writing into this slot, so that concurrent recorders cannot fragment each other's
    // payloads. Created on first write and then kept across reset() -- a cleared chain owns no buffers, so a retained
    // entry costs one map node and saves rebuilding the chain for an initiator that records here again.
    std::unordered_map<Device::ID, std::unique_ptr<pepp::bts::BufferChain>, pepp::handle_hash<Device::ID>> data;
    // Ordinals handed out in this slot, which is also the number of location-buffer entries in use. Decremented by
    // abort() to allowe reuse of that aborted slot. An ordinal whose recording has not committed yet reads back as a
    // tombstone (a program that immediately halts).
    u16 count = 0;

    // For each group, the ordinal of the first entry in that group (which is a full ProgramLocation).
    std::array<u16, LOCATION_GROUPS> group_first_ordinal{};
    // Bytes used by each group, which is where its next entry is appended.
    std::array<u16, LOCATION_GROUPS> group_used{};
    // Number of groups in use. Only the latest group can be appended to.
    u16 groups = 0;
    // The last entry written to the page, which is what the next one encodes against. A group's first entry is stored
    // whole, so this restarts from that entry whenever a group opens.
    tvm::ProgramLocation last_emitted{};
    // Recordings open in this slot, either 0 or 1. acknowledge() will not reclaim a slot while one is open to avoid a
    // UAF by the recorder.
    u16 open = 0;

    void reset(pepp::bts::BufferManager &mgr);
  };

  // --- Stencil dedup ---
  struct StencilEntry {
    u16 index = 0; // A subscript into _stencil_locations.
    u32 hit_count = 0;
    // The bytes of the promoted body. On a hash hit, we want to compare the actual bytes to avoid collisions.
    // This span avoids walking of the stencil chain on hit. While our size is really only a u16, it gets promoted to
    // size_t on account of being a span. Always downcast size to 16 bits before use.
    //
    // The pointer is safe to hold as long as the stencil chain is not cleared, and as long as a Buffer's data is not
    // moved out of.
    bits::span<const u8> body{};
  };

  // The stencil to which this body corresponds or a nullptr if the body must be written to the ring directly. The
  // pointer is only valid until the next promotion or call to resolve_body.
  const StencilEntry *resolve_body(bits::span<const u8> body);
  // True when the stencil recorded in `entry` holds exactly `body`. resolve_body keys stencils on a truncated
  // 32-bit hash, so a map hit alone does not prove the bodies match; this is what makes a collision safe.
  static bool stencil_matches(const StencilEntry &entry, bits::span<const u8> body);
  tvm::ProgramLocation flush_to_ring(Recording &rec, const StencilEntry *stencil);

  // This recording's data chain in the ringbuffer's head slot, creating the chain on first use.
  pepp::bts::BufferChain &data_chain(Recording &rec);

  // Advance _head to the next ring slot. Fires watermark callbacks as needed.
  void advance_slot();

  // Append one entry to the newest group, opening a new group as necessary.
  void emit_location(Node &node, tvm::ProgramLocation program);
  // True when the current group is full and no new groups can be allocated. Please advance to the next slot.
  bool slot_full(const Node &node) const;
  // Bytes still available for entries across this slot's remaining groups.
  static std::size_t room_left(const Node &node);
  // The bytes of `group` within the slot's location buffer.
  static u8 *group_base(Node &node, u16 group);

  // Read back the ProgramLocation stored at an ordinal in the location buffer.
  tvm::ProgramLocation read_location(const Node &node, u16 ordinal) const;
  // The group holding ordinal.
  static u16 group_of(const Node &node, u16 ordinal);

  // Every record looks up its node several times, so these mask rather than divide. See the constructor.
  Node &current_node() { return _ring[_head & _ring_mask]; }
  const Node &current_node() const { return _ring[_head & _ring_mask]; }
  const Node &node_at(size_t absolute_slot) const { return _ring[absolute_slot & _ring_mask]; }
  Node &node_at(size_t absolute_slot) { return _ring[absolute_slot & _ring_mask]; }

  // The node holding `absolute_slot`, or nullptr once the ring has moved on and a later slot took over that node --
  // i.e. non-null exactly when a Cursor naming that slot still refers to the entries it named when it was taken.
  //
  // Returns the node rather than a bool so that a caller needing both the test and the entries pays one modulo
  // instead of two. _ring.size() is a runtime value, so we would have to idiv twice.
  const Node *resident_node(size_t absolute_slot) const {
    const Node &node = node_at(absolute_slot);
    return node.slot == absolute_slot ? &node : nullptr;
  }
  // Entries readable at `absolute_slot`, or 0 when the ring has moved on and that slot is no longer resident.
  u16 count_at(size_t absolute_slot) const {
    const Node *node = resident_node(absolute_slot);
    return node == nullptr ? 0 : node->count;
  }

  std::shared_ptr<pepp::bts::BufferManager> _mgr;

  std::vector<Node> _ring;
  // _ring.size() - 1, which reduces an absolute slot to its node since the size is a power of two.
  std::size_t _ring_mask = 0;
  // _head and _tail may exceed the size of _ring, so always reduce them through node_at() / current_node().
  std::size_t _head = 0; // Next slot to write
  std::size_t _tail = 0; // Oldest unconsumed slot

  // Only devices that actually initiate accesses ever appear. That set is not known until a device starts
  // recording. Sizing this to the Device::ID space would allocate hundreds of idle std::vectors to serve the one or two
  // are actual initiators. Entries are created on first begin() and then kept, so a device's scratch buffers
  // keep their capacity across programs instead of reallocating on every instruction.
  std::unordered_map<Device::ID, Recording, pepp::handle_hash<Device::ID>> _recordings;
  // If non-nullptr, a cached pointer from _recordings for the open recording.
  Recording *_open = nullptr;

  // Stencils are only freed on TraceBuffer destruction to avoid lifetime management issues.
  std::unique_ptr<pepp::bts::BufferChain> _stencils;
  // Buffer::ID{0} hard-stops the interpreter with InvalidIBuffer. Claimed-but-unwritten entries point here instead, at
  // a program that contains only HALT. Always the first entry of the stencil chain, written by clear(), and doubles as
  // *CALLHALT's return address.
  tvm::ProgramLocation _tombstone{};
  std::unordered_map<u32, StencilEntry> _stencil_map;
  // A reverse lookup from StencilEntry::index to its body's bytes. Stencils are stored densely in promotion order.
  std::vector<pepp::bts::Buffer::Location> _stencil_locations;
  // Hashes seen once but not yet promoted. On second occurrence with
  // body.size() >= PROMOTION_THRESHOLD, the body is promoted to _stencils.
  std::unordered_set<u32> _pending_hashes;

  // Watermark callbacks are fired when the number of used ring slots crosses a threshold (0.0 to 1.0).
  struct Watermark {
    float threshold;
    WatermarkCallback callback;
    bool fired = false;
  };
  // Helper method to return a pointer to a Traceable given a Device::ID and notify it of tracing state changes.
  TraceableFinder _finder = [](Device::ID) -> Traceable * { return nullptr; };

  std::vector<Watermark> _watermarks;
  // Both indexed by Device::ID, whose underlying type is u8. 32 bytes each, and a lookup is a word load plus a bit
  // test and cheap enough to sit on the per-write path.
  std::bitset<256> _traced;
  std::bitset<256> _address_in_payload;

  // Accumulated performance counters.
  Footprint _footprint;
};

} // namespace tvm
