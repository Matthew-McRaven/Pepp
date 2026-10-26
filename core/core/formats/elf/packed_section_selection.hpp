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
#include <algorithm>
#include <concepts>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>
#include "core/formats/elf/packed_input_group.hpp"

namespace pepp::bts {
// One section of a PackedInputElfGroup as an argument to the selection predicate.
template <ElfBits B, ElfEndian E> struct InputElfSectionRef {
  InputElfSectionHandle handle;
  const PackedInputElfFile<B, E> &file;
  const PackedElfShdr<B, E> &header;
};

// Extract the sections of a group which satisfy a predicate, assigning contiguous indices to selected sections. Those
// indices are suitable for vectors/bitset. If the contents of the group change (e.g., new sections, deleted files), you
// will need to create a new selection and re-index any depedent data structures.
template <ElfBits B, ElfEndian E> class PackedSectionSelection {
public:
  using Group = PackedInputElfGroup<B, E>;
  using Ref = InputElfSectionRef<B, E>;

  // Extract all sections of the group, including the null section.
  explicit PackedSectionSelection(const Group &group)
      : PackedSectionSelection(group, [](const Ref &) { return true; }) {}
  // Defined in the class because MSVC cannot match an out-of-class constrained template to its declaration.
  template <std::predicate<const Ref &> Pred> PackedSectionSelection(const Group &group, Pred &&select) {
    _file_begin.reserve(group.size() + 1);
    _file_begin.push_back(0); // File 0 is reserved as an invalid/sentinel value.
    for (u32 id = 1; id <= group.size(); ++id) {
      const auto &file = group.file(ElfFileID(id));
      for (const auto handle : group.sections(ElfFileID(id))) {
        if (!select(Ref{handle, file, file.section_headers[handle.index]})) continue;
        else if (_handles.size() >= std::numeric_limits<u32>::max())
          throw std::length_error("PackedSectionSelection: too many sections");
        _handles.push_back(handle);
      }
      _file_begin.push_back(static_cast<u32>(_handles.size()));
    }
  }

  // The number of files the group had when this selection was made.
  u32 files() const noexcept { return static_cast<u32>(_file_begin.size() - 1); }
  u32 size() const noexcept { return static_cast<u32>(_handles.size()); }
  bool empty() const noexcept { return _handles.empty(); }

  // Throws std::out_of_range if position >= size().
  InputElfSectionHandle handle(u32 position) const;
  // nullopt if the section was not selected.
  std::optional<u32> position(InputElfSectionHandle handle) const;
  bool contains(InputElfSectionHandle handle) const { return position(handle).has_value(); }
  // All selected sections, or those of one file, in position order. Throws std::out_of_range unless 0 < id <= files().
  std::span<const InputElfSectionHandle> handles() const;
  std::span<const InputElfSectionHandle> handles(ElfFileID id) const;

private:
  std::vector<InputElfSectionHandle> _handles;
  // _handles[_file_begin[id - 1], _file_begin[id]) are the selected sections of a particulat file.
  std::vector<u32> _file_begin;
};

template <ElfBits B, ElfEndian E> InputElfSectionHandle PackedSectionSelection<B, E>::handle(u32 position) const {
  if (position >= _handles.size()) throw std::out_of_range("PackedSectionSelection: no section at this position");
  return _handles[position];
}

template <ElfBits B, ElfEndian E>
std::optional<u32> PackedSectionSelection<B, E>::position(InputElfSectionHandle handle) const {
  if (!handle || handle.file.value >= _file_begin.size()) return std::nullopt;
  // Handles are sorted by file then by section index. Limit search to the target file.
  const auto first = _handles.begin() + _file_begin[handle.file.value - 1];
  const auto last = _handles.begin() + _file_begin[handle.file.value];
  const auto it = std::lower_bound(first, last, handle);
  if (it == last || *it != handle) return std::nullopt;
  return static_cast<u32>(it - _handles.begin());
}

template <ElfBits B, ElfEndian E>
std::span<const InputElfSectionHandle> PackedSectionSelection<B, E>::handles() const {
  return _handles;
}

template <ElfBits B, ElfEndian E>
std::span<const InputElfSectionHandle> PackedSectionSelection<B, E>::handles(ElfFileID id) const {
  if (!id || id.value >= _file_begin.size()) throw std::out_of_range("PackedSectionSelection: no file with this ID");
  const u32 first = _file_begin[id.value - 1];
  return std::span(_handles).subspan(first, _file_begin[id.value] - first);
}
} // namespace pepp::bts
