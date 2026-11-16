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

#include "core/formats/elf/packed_section_selection.hpp"

pepp::bts::InputElfSectionHandle pepp::bts::PackedSectionSelection::handle(u32 position) const {
  if (position >= _handles.size()) throw std::out_of_range("PackedSectionSelection: no section at this position");
  return _handles[position];
}

std::optional<u32> pepp::bts::PackedSectionSelection::position(InputElfSectionHandle handle) const {
  if (!handle || handle.file.value >= _offsets.size() - 1) return std::nullopt;
  // Handles are sorted by file then by section index. Limit search to the target file.
  const auto first = _handles.begin() + _offsets[handle.file.value];
  const auto last = _handles.begin() + _offsets[handle.file.value + 1];
  const auto it = std::lower_bound(first, last, handle);
  if (it == last || *it != handle) return std::nullopt;
  return static_cast<u32>(it - _handles.begin());
}

std::span<const pepp::bts::InputElfSectionHandle> pepp::bts::PackedSectionSelection::handles() const {
  return _handles;
}

std::span<const pepp::bts::InputElfSectionHandle> pepp::bts::PackedSectionSelection::handles(ElfFileID id) const {
  if (!id || id.value >= _offsets.size() - 1) throw std::out_of_range("PackedSectionSelection: no file with this ID");
  const u32 first = _offsets[id.value];
  return std::span(_handles).subspan(first, _offsets[id.value + 1] - first);
}
