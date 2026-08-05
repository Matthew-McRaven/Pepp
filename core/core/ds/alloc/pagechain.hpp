#pragma once
#include "core/ds/alloc/paged.hpp"
#include "core/ds/opaque_handle.hpp"

namespace pepp::bts {

class BufferManager;

// A 2^16 byte slab of memory which is identifiable by a 16-bit ID.
class Buffer : public Slab<u8> {
public:
  using page_offset_t = u16;
  using ID = pepp::OpaqueHandle<struct BufferID, u16>;
  static constexpr size_t SIZE = 0x1'0000; // 2^16 bytes, or 64KiB.

  // An ID is (generation << INDEX_BITS) | index. The index selects a slot in the BufferManager; the generation says
  // *which occupant* of that slot. Reusing an index is mandatory -- ids are 16 bits and a long session will churn
  // through far more than 65536 buffers -- but an id left over from a previous occupant must not silently resolve to
  // whoever holds the slot now, which is what the generation half prevents. Both halves have to fit in 16 bits,
  // because the trace VM keeps a buffer id in half of a 32-bit register (IP.hi, DP.hi).
  //
  // 12/4 caps live buffers at 4095 which is 256 MiB of resident trace -- and gives a slot 16 occupants before an id can
  // alias. Allocation hands out free indices in FIFO order specifically to stretch that: an index is not reused until
  // every other free index has been, so 16 occupants is many ring wraps rather than 16 consecutive allocations. Retune
  // here if either bound bites.
  static constexpr u16 INDEX_BITS = 12;
  static constexpr u16 INDEX_MASK = (1u << INDEX_BITS) - 1;
  static constexpr u16 GENERATION_MASK = 0xFFFF >> INDEX_BITS;
  // Index 0 is never handed out, which is what makes ID{0} reliably mean "no buffer"
  static constexpr u16 MAX_LIVE_BUFFERS = INDEX_MASK;
  static constexpr u16 index_of(ID id) noexcept { return id.value & INDEX_MASK; }
  static constexpr u16 generation_of(ID id) noexcept { return id.value >> INDEX_BITS; }
  static constexpr ID make_id(u16 index, u16 generation) noexcept {
    return ID{static_cast<u16>(((generation & GENERATION_MASK) << INDEX_BITS) | (index & INDEX_MASK))};
  }

  Buffer(const Buffer &) = delete;
  ID id() const { return _id; }
  struct Location {
    Buffer::ID id;
    Buffer::page_offset_t offset;
  };
  // Returns a Location which points to the next free by in the buffer.
  // Useful when appending data to the buffer.
  Location location() const { return Location{_id, (u16)used_capacity()}; }

private:
  friend class BufferManager;
  Buffer(ID id);
  ID _id;
};

consteval void allow_opaque_handle_increment(Buffer::ID);

// Gives the appearance (API) of being a buffer, but really is a contiguous chain of buffers.
// Useful for representing traces of tvm::Interpreter programs, which can insert "swaps" between buffers easily.
// This class can only be constructed by a BufferManager. When this chain is destroyed, it will release
// all buffers back into the manager's pool.
class BufferChain {
public:

  ~BufferChain() noexcept;
  // Return pages from the chain to the manager's pool.
  void clear() noexcept;
  // Copy elements into next free space, advancing size.
  // Require data be aligned % align (padding at start) with pad (padding at end).
  // Both align and pad are in element counts, not bytes.
  Buffer::Location append(bits::span<const u8> data, size_t byte_align = 0, size_t byte_pad = 0, u8 fill = 0);
  // An append will all elements set to `fill`.
  Buffer::Location allocate_initialized(size_t size, u8 fill = 0);
  // Bump size without modifying underlying data.
  Buffer::Location allocate_uninitialized(size_t size);
  // Combine a writable span with the buffer location of allocate_uninitialized.
  struct Reservation {
    Buffer::Location loc;
    bits::span<u8> bytes;
  };
  // Call allocate_uninitialized and return a pointer to those bytes.
  Reservation reserve(size_t size);
  u16 buffer_count() const;
  // Given a buffer ID, return the pointer to that buffer
  Buffer *buffer(Buffer::ID id);
  // given an index into the chain (e.g., and index of _buf), return the pointer to that buffer.
  Buffer *buffer(size_t index);
  // Return the successor buffer's ID in the chain, or Buffer::ID{0} if it is the last buffer/not found.
  Buffer::ID successor(Buffer::ID id);
  // Return the predecessor buffer's ID in the chain, or Buffer::ID{0} if it is the first buffer/not found.
  Buffer::ID predecessor(Buffer::ID id);
  // Ensure the tail buffer has at least `bytes` of free space.
  // If not, advance to a fresh buffer. Use before multiple appends that must land in the same buffer.
  void ensure_capacity(size_t bytes);

private:
  friend class BufferManager;
  BufferChain(BufferManager *mgr);
  Buffer *tail();
  // Request a new buffer from the manager, updating tail and _buffer_count.
  // If _head is empty, also update _head.
  void append_buffer();
  u16 _buffer_count = 0;
  BufferManager *_mgr = nullptr;
  std::vector<Buffer *> _bufs;
  std::unordered_map<Buffer::ID, Buffer::ID, pepp::handle_hash<Buffer::ID>> _successor;
  std::unordered_map<Buffer::ID, Buffer::ID, pepp::handle_hash<Buffer::ID>> _predecessor;
};

// Class which manages the lifetimes of Buffers and provides ID<=>Buffer mapping.
// It doesn't enforce any ownership rules, so you could free a still-live buffer.
// It's a bit of a half-step between the PagedPool and PagedAllocator.
// Like PagedPool, it doesn't care what you do with the pages, but this class derives from Slab rather than Page to give
// nice allocation APIs.
// Unlike PagedAllocator, it doesn't care about contiguous allocation, nor does it give you global offsets.
class BufferManager {
public:
  std::unique_ptr<BufferChain> alloc_chain();
  Buffer *alloc_buffer();
  Buffer *find(Buffer::ID);
  const Buffer *find(Buffer::ID) const;
  // Free a single buffer. This could cause a use-after-free if the buffer is still in use by another chain.
  // Buffers are prone to frequent re-use (a free followed by an alloc might return the same data!).
  void free_buffer(Buffer::ID);

  u16 free_buffers() const noexcept;
  u16 allocated_buffers() const noexcept;

private:
  // But 4095 is a valid index you say? Well, indices higher than 2<<INDEX_BITS could never refer to a valid slot,
  // because they would be out-of-range. So use on of those as a designated "no index" value.
  static constexpr u16 NO_INDEX = 0xFFFF;

  // One entry per index ever handed out, up to Buffer::MAX_LIVE_BUFFERS.
  // A slot keeps its 64 KiB storage across a free so the allocation is reused.
  // A generation id is prepended to the index to to form a 16 bit id.
  // Accesses to the same index with a different generation ID will return a nullptr to prevent use-after frees.
  struct Slot {
    bool live = false;
    // Bumped on free, so ids naming the previous occupant stop resolving. Wraps; it only has to differ from the ids
    // still in flight, not be globally unique.
    u16 generation = 0;
    // Intrusive free list, threaded through the slots so the free set needs no container of its own. Meaningful only
    // while `live` is false.
    u16 next_free = NO_INDEX;
    std::unique_ptr<Buffer> storage;
  };
  // Don't "hand out" ID=0, so we can use is as a nullptr,  With 2^12-2 buffers, we can address ~256MB of code+data in a
  // single ringbuffer. Checking if a buffer exists becomes a bounds check on the index and a generation check rather
  // than a hash table lookup.
  std::vector<Slot> _slots;
  // Follow a FIFO/queue reuse rather than LIFO/stack reuse. This stretches time between free/reuse of a slot, hopefully
  // leveling out generations too.
  u16 _free_head = NO_INDEX, _free_tail = NO_INDEX;
  u16 _live_count = 0, _free_count = 0;
};

} // namespace pepp::bts
