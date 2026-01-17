/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "packed_storage.hpp"
#include "packed_ops.hpp"

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

size_t pepp::bts::NullStorage::append(bits::span<const u8>) { return 0; }

size_t pepp::bts::NullStorage::allocate(size_t, u8) { return 0; }

void pepp::bts::NullStorage::set(size_t, bits::span<const u8>) {}

bits::span<u8> pepp::bts::NullStorage::get(size_t, size_t) noexcept { return {}; }

bits::span<const u8> pepp::bts::NullStorage::get(size_t, size_t) const noexcept { return {}; }

size_t pepp::bts::NullStorage::size() const noexcept { return 0; }

void pepp::bts::NullStorage::clear(size_t) {}

size_t pepp::bts::NullStorage::calculate_layout(std::vector<LayoutItem> &, size_t dst_offset) const {
  return dst_offset;
}

size_t pepp::bts::NullStorage::find(bits::span<const u8>) const noexcept { return 0; }

size_t pepp::bts::NullStorage::strlen(size_t) const noexcept { return 0; }

void pepp::bts::CombiningStorage::add_section(ConstAnyPackedElfPtr elf, u16 section_index) {
  _entries.emplace_back(elf, section_index);
}

size_t pepp::bts::CombiningStorage::append(bits::span<const u8>) { return -1; }

size_t pepp::bts::CombiningStorage::allocate(size_t, u8) { return -1; }

void pepp::bts::CombiningStorage::set(size_t, bits::span<const u8>) { return; }

bits::span<u8> pepp::bts::CombiningStorage::get(size_t, size_t) noexcept {
  throw std::logic_error("CombiningStorage::get not implemented");
}

bits::span<const u8> pepp::bts::CombiningStorage::get(size_t offset, size_t length) const noexcept {
  bool first = true;
  for (const auto &[elf, section_index] : _entries) {
    size_t align = sh_align(elf, section_index);
    std::shared_ptr<const pepp::bts::AStorage> data = section_data(elf, section_index);
    if (offset < data->size()) return data->get(offset, length);
    if (first) {
      if (offset < data->size()) return {};
      else offset -= data->size(), first &= false;
    } else {
      auto aligned_size = bits::align_up(data->size(), align);
      if (offset < aligned_size) return {};
      else offset -= aligned_size;
    }
  }
  return {};
}

size_t pepp::bts::CombiningStorage::size() const noexcept {
  size_t total_size = 0;
  bool first = true;
  for (const auto &[elf, section_index] : _entries) {
    size_t align = sh_align(elf, section_index);
    std::shared_ptr<const pepp::bts::AStorage> data = section_data(elf, section_index);
    // Pray that the containing's sections sh_align is correct
    if (!first) total_size = bits::align_up(total_size, align);
    total_size += data->size(), first &= false;
  }
  return total_size;
}

void pepp::bts::CombiningStorage::clear(size_t) { return; }

size_t pepp::bts::CombiningStorage::calculate_layout(std::vector<LayoutItem> &layout, size_t dst_offset) const {
  size_t rolling_offset = dst_offset;
  bool first = true;
  for (const auto &[elf, section_index] : _entries) {
    size_t align = sh_align(elf, section_index);
    std::shared_ptr<const pepp::bts::AStorage> data = section_data(elf, section_index);
    // Pray that the containing's sections sh_align is correct
    if (!first) rolling_offset = bits::align_up(rolling_offset, align);
    rolling_offset = data->calculate_layout(layout, rolling_offset), first &= false;
  }
  return rolling_offset;
}

size_t pepp::bts::CombiningStorage::find(bits::span<const u8> needle) const noexcept {
  size_t rolling_offset = 0;
  bool first = true;
  for (const auto &[elf, section_index] : _entries) {
    size_t align = sh_align(elf, section_index);
    std::shared_ptr<const pepp::bts::AStorage> data = section_data(elf, section_index);
    auto local_offset = data->find(needle);
    if (local_offset != -1u) return rolling_offset + local_offset;
    else if (first) rolling_offset += data->size(), first &= false;
    else rolling_offset += bits::align_up(data->size(), align);
  }
  return -1u;
}

size_t pepp::bts::CombiningStorage::strlen(size_t offset) const noexcept {
  bool first = true;
  for (const auto &[elf, section_index] : _entries) {
    size_t align = sh_align(elf, section_index);
    std::shared_ptr<const pepp::bts::AStorage> data = section_data(elf, section_index);
    if (offset < data->size()) return data->strlen(offset);
    if (first) {
      if (offset < data->size()) return 0;
      else offset -= data->size(), first &= false;
    } else {
      auto aligned_size = bits::align_up(data->size(), align);
      if (offset < aligned_size) return 0;
      else offset -= aligned_size;
    }
  }
  return 0;
}
