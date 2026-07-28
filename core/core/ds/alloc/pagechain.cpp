#include "pagechain.hpp"

pepp::bts::BufferChain::BufferChain(BufferManager *mgr) : _mgr(mgr) {}

pepp::bts::BufferChain::~BufferChain() noexcept { clear(); }

void pepp::bts::BufferChain::clear() noexcept {
  _successor.clear();
  for (auto *buf : _bufs) _mgr->free_buffer(buf->id());
  _bufs.clear();
  _buffer_count = 0;
}

pepp::bts::Buffer::Location pepp::bts::BufferChain::allocate_uninitialized(size_t size) {
  auto buf = tail();
  if (!buf->can_fit(size)) {
    append_buffer();
    buf = tail();
  }
  const auto off = buf->allocate_uninitialized(size);
  return Buffer::Location{buf->id(), (u16)off};
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
  if (!_bufs.empty()) _successor[_bufs.back()->id()] = buf->id();
  _bufs.push_back(buf);
}

std::unique_ptr<pepp::bts::BufferChain> pepp::bts::BufferManager::alloc_chain() {
  return std::unique_ptr<BufferChain>(new BufferChain(this));
}

pepp::bts::Buffer *pepp::bts::BufferManager::alloc_buffer() {
  if (_free_pool.empty()) {
    // Allocate a fresh buffer if pool is empty
    auto buf = std::unique_ptr<Buffer>(new Buffer(_next_id++));
    auto ret = buf.get();
    _allocated[ret->id()] = std::move(buf);
    return ret;
  } else {
    // Otherwise take most recently freed buffer and re-use it.
    auto buf = std::move(_free_pool.back());
    auto ret = buf.get();
    _free_pool.pop_back();
    _allocated[ret->id()] = std::move(buf);
    return ret;
  }
}

pepp::bts::Buffer *pepp::bts::BufferManager::find(Buffer::ID id) {
  if (auto it = _allocated.find(id); it != _allocated.end()) return it->second.get();
  else return nullptr;
}

const pepp::bts::Buffer *pepp::bts::BufferManager::find(Buffer::ID id) const {
  if (auto it = _allocated.find(id); it != _allocated.end()) return it->second.get();
  else return nullptr;
}

void pepp::bts::BufferManager::free_buffer(Buffer::ID id) {
  if (_allocated.contains(id)) {
    // Extract value from _allocated and move it into _free_pool, allowing this data to be re-used.
    auto buf = std::move(_allocated[id]);
    _free_pool.push_back(std::move(buf));
    // Then remove the lookup from ID->Buffer, since it is no longer in use.
    _allocated.erase(id);
  }
}

u16 pepp::bts::BufferManager::allocated_buffers() const noexcept { return _allocated.size(); }

u16 pepp::bts::BufferManager::free_buffers() const noexcept { return _free_pool.size(); }

pepp::bts::Buffer::Buffer(ID id) : Slab<u8>(SIZE), _id(id) {}
