#include "pagechain.hpp"
#include <stdexcept>
#include <utility>

pepp::bts::BufferChain::BufferChain(BufferManager *mgr) : _mgr(mgr) {}

pepp::bts::BufferChain::~BufferChain() noexcept { clear(); }

void pepp::bts::BufferChain::clear() noexcept {
  _successor.clear();
  _predecessor.clear();
  for (auto *buf : _bufs) _mgr->free_buffer(buf->id());
  _bufs.clear();
  _buffer_count = 0;
}

pepp::bts::Buffer::Location pepp::bts::BufferChain::allocate_uninitialized(size_t size) { return reserve(size).loc; }

pepp::bts::BufferChain::Reservation pepp::bts::BufferChain::reserve(size_t size) {
  auto buf = tail();
  if (!buf->can_fit(size)) {
    append_buffer();
    buf = tail();
  }
  const auto off = buf->allocate_uninitialized(size);
  return Reservation{Buffer::Location{buf->id(), (u16)off}, buf->span().subspan(off, size)};
}

pepp::bts::Buffer::Location pepp::bts::BufferChain::allocate_initialized(size_t size, u8 fill) {
  auto buf = tail();
  if (!buf->can_fit(size)) {
    append_buffer();
    buf = tail();
  }
  const auto off = buf->allocate_initialized(size, fill);
  return Buffer::Location{buf->id(), (u16)off};
}

pepp::bts::Buffer::Location pepp::bts::BufferChain::append(bits::span<const u8> data, size_t byte_align,
                                                           size_t byte_pad, u8 fill) {
  auto buf = tail();
  if (!buf->can_fit(data, byte_align, byte_pad)) {
    append_buffer();
    buf = tail();
  }
  const auto off = buf->append(data, byte_align, byte_pad, fill);
  return Buffer::Location{buf->id(), (u16)off};
}

u16 pepp::bts::BufferChain::buffer_count() const { return _buffer_count; }

pepp::bts::Buffer::ID pepp::bts::BufferChain::successor(Buffer::ID id) {
  if (auto it = _successor.find(id); it == _successor.end()) return Buffer::ID{0};
  else return it->second;
}

pepp::bts::Buffer::ID pepp::bts::BufferChain::predecessor(Buffer::ID id) {
  if (auto it = _predecessor.find(id); it == _predecessor.end()) return Buffer::ID{0};
  else return it->second;
}

void pepp::bts::BufferChain::ensure_capacity(size_t bytes) {
  auto buf = tail();
  if (!buf->can_fit(bytes)) append_buffer();
}

pepp::bts::Buffer *pepp::bts::BufferChain::buffer(size_t index) {
  if (index >= _bufs.size()) return nullptr;
  return _bufs[index];
}

pepp::bts::Buffer *pepp::bts::BufferChain::buffer(Buffer::ID id) {
  for (auto *buf : _bufs)
    if (buf->id() == id) return buf;
  return nullptr;
}

pepp::bts::Buffer *pepp::bts::BufferChain::tail() {
  // If empty, request buffer from mgr
  if (_bufs.empty()) append_buffer();
  return _bufs.back();
}

void pepp::bts::BufferChain::append_buffer() {
  auto buf = _mgr->alloc_buffer();
  _buffer_count++;
  if (!_bufs.empty()) {
    _successor[_bufs.back()->id()] = buf->id();
    _predecessor[buf->id()] = _bufs.back()->id();
  }
  _bufs.push_back(buf);
}

std::unique_ptr<pepp::bts::BufferChain> pepp::bts::BufferManager::alloc_chain() {
  return std::unique_ptr<BufferChain>(new BufferChain(this));
}

pepp::bts::Buffer *pepp::bts::BufferManager::alloc_buffer() {
  if (_free_head != NO_INDEX) {
    // Reuse a slot. Its generation was already bumped when it was freed, so the id handed out here cannot match one
    // that named the previous occupant.
    const u16 index = _free_head;
    auto &slot = _slots[index];
    // Take the slot's next_free as the new head and remove the slot from the free list
    _free_head = std::exchange(slot.next_free, NO_INDEX);
    if (_free_head == NO_INDEX) _free_tail = NO_INDEX;
    slot.live = true;
    slot.storage->_id = Buffer::make_id(index, slot.generation);
    --_free_count, ++_live_count;
    return slot.storage.get();
  }

  // Do not allow slot 0 to be handed out, so Buffer::ID of 0 can effectively be a nullptr
  if (_slots.empty()) _slots.emplace_back();
  else if (_slots.size() > Buffer::MAX_LIVE_BUFFERS)
    throw std::runtime_error("BufferManager: out of buffer indices; the ring is retaining more trace than the id "
                             "space allows");
  const u16 index = static_cast<u16>(_slots.size());
  _slots.emplace_back();
  auto &slot = _slots[index];
  slot.storage = std::unique_ptr<Buffer>(new Buffer(Buffer::make_id(index, 0)));
  slot.live = true;
  ++_live_count;
  return slot.storage.get();
}

pepp::bts::Buffer *pepp::bts::BufferManager::find(Buffer::ID id) {
  return const_cast<Buffer *>(std::as_const(*this).find(id));
}

const pepp::bts::Buffer *pepp::bts::BufferManager::find(Buffer::ID id) const {
  // Treat of-of-range indices as a nullptr rather than throwing.
  if (const u16 index = Buffer::index_of(id); index == 0 || index >= _slots.size()) return nullptr;
  // Check if the slot is freed, or if it has been recycled since id was minted. Treat both as a nullptr.
  else if (const auto &slot = _slots[index]; !slot.live || slot.generation != Buffer::generation_of(id)) return nullptr;
  else return slot.storage.get();
}

void pepp::bts::BufferManager::free_buffer(Buffer::ID id) {
  const u16 index = Buffer::index_of(id);
  if (index == 0 || index >= _slots.size()) return;
  auto &slot = _slots[index];
  // Ignore a double free, and a free through a stale id -- which would otherwise evict the slot's current occupant.
  if (!slot.live || slot.generation != Buffer::generation_of(id)) return;

  // Reset the allocation cursor so a recycled buffer is as empty as a fresh one. Without this a buffer carries its
  // old used_capacity() back out of the pool. This only resets the capacity counter, it does not 0-out bytes.
  slot.storage->clear();
  slot.live = false;
  // Increment the generation modulo the max number of generations. Since it's a power of 2, we can use bitops rather
  // than idiv.
  slot.generation = static_cast<u16>((slot.generation + 1) & Buffer::GENERATION_MASK);

  // Append to the tail so indices come back round-robin rather than LIFO.
  slot.next_free = NO_INDEX;
  if (_free_tail == NO_INDEX) _free_head = _free_tail = index;
  else _slots[_free_tail].next_free = index, _free_tail = index;
  --_live_count, ++_free_count;
}

u16 pepp::bts::BufferManager::allocated_buffers() const noexcept { return _live_count; }

u16 pepp::bts::BufferManager::free_buffers() const noexcept { return _free_count; }

pepp::bts::Buffer::Buffer(ID id) : Slab<u8>(SIZE), _id(id) {}
