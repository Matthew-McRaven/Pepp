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
// Useful for representing traces of RegisterBlaster programs, which can insert "swaps" between buffers easily.
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
  Buffer ::Location allocate_uninitialized(size_t size);
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
  std::vector<std::unique_ptr<Buffer>> _free_pool;
  std::unordered_map<Buffer::ID, std::unique_ptr<Buffer>, pepp::handle_hash<Buffer::ID>> _allocated;
  Buffer::ID _next_id = Buffer::ID{1};
};

} // namespace pepp::bts
