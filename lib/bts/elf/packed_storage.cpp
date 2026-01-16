#include "packed_storage.hpp"
#include <system_error>
#include "packed_ops.hpp"
// Needed for memory-mapped storage
#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#elif defined(__unix__) || defined(__APPLE__)
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include <fstream>

pepp::bts::AStorage::~AStorage() = default;

size_t pepp::bts::BlockStorage::append(bits::span<const u8> data) {
  auto offset = _storage.size();
  _storage.insert(_storage.end(), data.begin(), data.end());
  return offset;
}

size_t pepp::bts::BlockStorage::allocate(size_t size, u8 fill) {
  auto ret = _storage.size();
  _storage.resize(ret + size, fill);
  return ret;
}

void pepp::bts::BlockStorage::set(size_t offset, bits::span<const u8> data) {
  if (offset + data.size() > _storage.size()) throw std::out_of_range("BlockStorage::set out of range");
  std::memcpy(_storage.data() + offset, data.data(), data.size());
}

bits::span<u8> pepp::bts::BlockStorage::get(size_t offset, size_t length) noexcept {
  if (offset + length > _storage.size()) return {};
  return bits::span<u8>((u8 *)_storage.data() + offset, length);
}

bits::span<const u8> pepp::bts::BlockStorage::get(size_t offset, size_t length) const noexcept {
  if (offset + length > _storage.size()) return {};
  return bits::span<const u8>((const u8 *)_storage.data() + offset, length);
}

size_t pepp::bts::BlockStorage::size() const noexcept { return _storage.size(); }

void pepp::bts::BlockStorage::clear(size_t reserve) {
  _storage.clear();
  if (reserve > 0) _storage.reserve(reserve);
}

size_t pepp::bts::BlockStorage::calculate_layout(std::vector<LayoutItem> &layout, size_t dst_offset) const {
  if (_storage.empty()) return dst_offset;
  layout.emplace_back(LayoutItem{dst_offset, bits::span<const u8>{(const u8 *)_storage.data(), _storage.size()}});
  return dst_offset + _storage.size();
}

size_t pepp::bts::BlockStorage::find(bits::span<const u8> needle) const noexcept {
  auto it = std::search(_storage.begin(), _storage.end(), needle.begin(), needle.end());
  return (it == _storage.end()) ? 0 : static_cast<std::size_t>(it - _storage.begin());
}

size_t pepp::bts::BlockStorage::strlen(size_t offset) const noexcept {
  const char *start = (const char *)_storage.data() + offset, *end = start;
  while (end < (const char *)_storage.data() + _storage.size() && *end != '\0') ++end;
  return end - start;
}

size_t pepp::bts::PagedStorage::append(bits::span<const u8> data) { return _allocator.append(data); }

size_t pepp::bts::PagedStorage::allocate(size_t size, u8 fill) { return _allocator.allocate_initialized(size, fill); }

void pepp::bts::PagedStorage::set(size_t offset, bits::span<const u8> data) {
  while (data.size() > 0) {
    auto [page_index, page_offset] = _allocator.indices_for_offset(offset);
    auto &page = _allocator.page(page_index);
    auto to_write = std::min<size_t>(data.size(), page.size() - page_offset);
    std::memcpy(page.data() + page_offset, data.data(), to_write);
    offset += to_write, data = data.subspan(to_write);
  }
}

bits::span<u8> pepp::bts::PagedStorage::get(size_t offset, size_t length) noexcept {
  return _allocator.get(offset, length);
}

bits::span<const u8> pepp::bts::PagedStorage::get(size_t offset, size_t length) const noexcept {
  return _allocator.get(offset, length);
}

size_t pepp::bts::PagedStorage::size() const noexcept { return _allocator.size(); }

void pepp::bts::PagedStorage::clear(size_t) { _allocator.clear(); }

size_t pepp::bts::PagedStorage::calculate_layout(std::vector<LayoutItem> &layout, size_t dst_offset) const {
  if (_allocator.size() == 0) return dst_offset;
  for (const auto &page : _allocator.pages()) {
    if (page.size() == 0) continue;
    layout.emplace_back(LayoutItem{dst_offset, bits::span<const u8>{page.data(), static_cast<size_t>(page.size())}});
    dst_offset += page.size();
  }
  return dst_offset;
}

size_t pepp::bts::PagedStorage::find(bits::span<const u8> data) const noexcept {
  size_t offset = 0;
  for (const auto &page : _allocator.pages()) {
    auto it = std::search(page.data(), page.data() + page.size(), data.begin(), data.end());
    if (it != page.data() + page.size()) return offset + static_cast<size_t>(it - page.data());
    offset += page.size();
  }
  return offset;
}

size_t pepp::bts::PagedStorage::strlen(size_t offset) const noexcept {
  if (offset >= _allocator.size()) return 0;
  auto [start_page_index, start_page_offset] = _allocator.indices_for_offset(offset);
  size_t length = 0;
  while (start_page_index < _allocator.pages().size()) {
    auto &page = _allocator.page(start_page_index);
    for (size_t it = start_page_offset; it < page.size(); it++)
      if (page.data()[it] == '\0') return length;
      else length++;
    start_page_offset = 0, start_page_index++;
    if (start_page_index >= _allocator.pages().size()) break;
  }
  return length;
}

pepp::bts::MemoryMapped::MemoryMapped(std::shared_ptr<MappedFile::Slice> slice) : _slice(slice) {}

size_t pepp::bts::MemoryMapped::append(bits::span<const u8>) {
  throw std::runtime_error("MemoryMapped storage does not support append()");
}

size_t pepp::bts::MemoryMapped::allocate(size_t, u8) {
  throw std::runtime_error("MemoryMapped storage does not support allocate()");
}

void pepp::bts::MemoryMapped::set(size_t offset, bits::span<const u8> data) {
  if (_slice->readonly()) return;
  auto sp = _slice->get();
  if (sp.size() == 0) throw std::runtime_error("MemoryMapped storage is not mapped");
  else if (offset + data.size() > sp.size()) throw std::out_of_range("MemoryMapped::set out of range");

  std::memcpy(const_cast<u8 *>(sp.data()) + offset, data.data(), data.size());
}

bits::span<u8> pepp::bts::MemoryMapped::get(size_t offset, size_t length) noexcept {
  auto sp = _slice->get();
  return sp.subspan(offset, length);
}

bits::span<const u8> pepp::bts::MemoryMapped::get(size_t offset, size_t length) const noexcept {
  auto sp = _slice->get();
  return sp.subspan(offset, length);
}

size_t pepp::bts::MemoryMapped::size() const noexcept { return _slice->size(); }

void pepp::bts::MemoryMapped::clear(size_t) {
  // TODO: ?? maybe this should free?
}

size_t pepp::bts::MemoryMapped::calculate_layout(std::vector<LayoutItem> &layout, size_t dst_offset) const {
  auto sp = _slice->get();
  layout.emplace_back(LayoutItem{dst_offset, sp});
  return dst_offset + sp.size();
}

size_t pepp::bts::MemoryMapped::find(bits::span<const u8> data) const noexcept {
  auto sp = _slice->get();
  auto it = std::search(sp.begin(), sp.end(), data.begin(), data.end());
  if (it == sp.end()) return 0;
  return static_cast<std::size_t>(it - sp.begin());
}

size_t pepp::bts::MemoryMapped::strlen(size_t offset) const noexcept {
  auto sp = _slice->get();
  const char *start = reinterpret_cast<const char *>(sp.data() + offset), *end = start;
  while (end < reinterpret_cast<const char *>(sp.data() + sp.size()) && *end != '\0') ++end;
  return end - start;
}
