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

#pragma once
#include <concepts>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "core/ds/opaque_handle.hpp"
#include "core/formats/elf/packed_elf.hpp"
#include "core/math/geom/interval.hpp"

namespace pepp::bts {
// Identifies one file within a PackedInputElfGroup. IDs start at 1, so a default-constructed ID is invalid.
using ElfFileID = pepp::OpaqueHandle<struct ElfFileTag, u16>;

// Identifies a section or segment within a PackedInputElfGroup, packing its file ID into the upper 16 bits and its
// index within that file into the lower 16.
using ElfSectionHandle = pepp::OpaqueHandle<struct ElfSectionTag, u32>;
using ElfSegmentHandle = pepp::OpaqueHandle<struct ElfSegmentTag, u32>;
template <class H>
concept ElfItemHandle = std::same_as<H, ElfSectionHandle> || std::same_as<H, ElfSegmentHandle>;

template <ElfItemHandle H> constexpr H make_elf_handle(ElfFileID file, u16 index) noexcept {
  return H((u32(file.value) << 16) | index);
}
template <ElfItemHandle H> constexpr ElfFileID elf_file_of(H handle) noexcept {
  return ElfFileID(static_cast<u16>(handle.value >> 16));
}
template <ElfItemHandle H> constexpr u16 elf_index_of(H handle) noexcept { return static_cast<u16>(handle.value); }

// A group of read-only ELF files which share a word size and byte order, such as every input to a single load.
// Handles name sections and segments across the whole group, so callers need not track which file each came from.
template <ElfBits B, ElfEndian E> class PackedInputElfGroup {
public:
  using File = PackedInputElfFile<B, E>;
  using Shdr = PackedElfShdr<B, E>;
  using Phdr = PackedElfPhdr<B, E>;

  // Takes ownership of file, returning the ID that its handles will use.
  ElfFileID add(std::unique_ptr<File> file);
  // Throws if the file cannot be read, or its class or byte order is not B / E.
  ElfFileID open(std::string path) { return add(std::make_unique<File>(std::move(path))); }

  std::size_t size() const noexcept { return _files.size(); }
  const File &file(ElfFileID id) const;
  // Handles for every section / segment of a file in index order, including the null section at index 0.
  std::vector<ElfSectionHandle> sections(ElfFileID id) const;
  std::vector<ElfSegmentHandle> segments(ElfFileID id) const;

  // All lookups throw std::out_of_range if the handle does not name an item in this group.
  const Shdr &header(ElfSectionHandle handle) const;
  const Phdr &header(ElfSegmentHandle handle) const;
  std::shared_ptr<const AStorage> data(ElfSectionHandle handle) const;

private:
  std::vector<std::unique_ptr<File>> _files;
};

template <ElfBits B, ElfEndian E> ElfFileID PackedInputElfGroup<B, E>::add(std::unique_ptr<File> file) {
  if (!file) throw std::invalid_argument("PackedInputElfGroup::add: file must be non-null");
  else if (_files.size() >= std::numeric_limits<ElfFileID::underlying_type>::max())
    throw std::length_error("PackedInputElfGroup::add: too many files");
  _files.push_back(std::move(file));
  return ElfFileID(static_cast<ElfFileID::underlying_type>(_files.size()));
}

template <ElfBits B, ElfEndian E> auto PackedInputElfGroup<B, E>::file(ElfFileID id) const -> const File & {
  if (!id || id.value > _files.size()) throw std::out_of_range("PackedInputElfGroup: no file with this ID");
  return *_files[id.value - 1];
}

template <ElfBits B, ElfEndian E>
std::vector<ElfSectionHandle> PackedInputElfGroup<B, E>::sections(ElfFileID id) const {
  const auto count = file(id).section_headers.size();
  std::vector<ElfSectionHandle> ret(count);
  for (std::size_t it = 0; it < count; ++it) ret[it] = make_elf_handle<ElfSectionHandle>(id, static_cast<u16>(it));
  return ret;
}

template <ElfBits B, ElfEndian E>
std::vector<ElfSegmentHandle> PackedInputElfGroup<B, E>::segments(ElfFileID id) const {
  const auto count = file(id).program_headers.size();
  std::vector<ElfSegmentHandle> ret(count);
  for (std::size_t it = 0; it < count; ++it) ret[it] = make_elf_handle<ElfSegmentHandle>(id, static_cast<u16>(it));
  return ret;
}

template <ElfBits B, ElfEndian E>
auto PackedInputElfGroup<B, E>::header(ElfSectionHandle handle) const -> const Shdr & {
  const auto &f = file(elf_file_of(handle));
  const auto index = elf_index_of(handle);
  if (index >= f.section_headers.size()) throw std::out_of_range("PackedInputElfGroup: no such section");
  return f.section_headers[index];
}

template <ElfBits B, ElfEndian E>
auto PackedInputElfGroup<B, E>::header(ElfSegmentHandle handle) const -> const Phdr & {
  const auto &f = file(elf_file_of(handle));
  const auto index = elf_index_of(handle);
  if (index >= f.program_headers.size()) throw std::out_of_range("PackedInputElfGroup: no such segment");
  return f.program_headers[index];
}

template <ElfBits B, ElfEndian E>
std::shared_ptr<const AStorage> PackedInputElfGroup<B, E>::data(ElfSectionHandle handle) const {
  const auto &f = file(elf_file_of(handle));
  const auto index = elf_index_of(handle);
  if (index >= f.section_data.size()) throw std::out_of_range("PackedInputElfGroup: no such section");
  return f.section_data[index];
}

// Extract the PT_LOAD-able segments from the group, returning their virtual memory address spans and segment handles.
template <ElfBits B, ElfEndian E>
std::vector<std::pair<ElfSegmentHandle, pepp::core::Interval<word<B>>>>
loadable_segments(const PackedInputElfGroup<B, E> &group) {
  std::vector<std::pair<ElfSegmentHandle, pepp::core::Interval<word<B>>>> ret;
  for (std::size_t id = 1; id <= group.size(); ++id) {
    for (const auto handle : group.segments(ElfFileID(static_cast<ElfFileID::underlying_type>(id)))) {
      const auto &phdr = group.header(handle);
      if (phdr.p_type != bits::to_underlying(SegmentType::PT_LOAD) || phdr.p_memsz == 0) continue;
      const word<B> start = phdr.p_vaddr, size = phdr.p_memsz;
      if (size - 1 > std::numeric_limits<word<B>>::max() - start)
        throw std::runtime_error("loadable_segments: segment extends past the end of the address space");
      ret.emplace_back(handle, pepp::core::Interval<word<B>>(start, start + (size - 1)));
    }
  }
  return ret;
}

using PackedInputElfGroupLE32 = PackedInputElfGroup<ElfBits::b32, ElfEndian::le>;
using PackedInputElfGroupBE32 = PackedInputElfGroup<ElfBits::b32, ElfEndian::be>;
using PackedInputElfGroupLE64 = PackedInputElfGroup<ElfBits::b64, ElfEndian::le>;
using PackedInputElfGroupBE64 = PackedInputElfGroup<ElfBits::b64, ElfEndian::be>;
} // namespace pepp::bts
