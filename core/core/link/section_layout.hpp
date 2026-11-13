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
#include <functional>
#include <vector>
#include "core/langs/ldscript/ir.hpp"
#include "core/link/diagnostics.hpp"
#include "core/link/section_placements.hpp"

namespace pepp::link {

// Represents the memory that an output section occupies.
struct OutputRange {
  u64 address = 0;
  u64 size = 0;
  u64 alignment = 1;
  // True if the output section has no contents, so that it is left out of the output file.
  bool empty = false;
  bool operator==(const OutputRange &) const = default;
};

// Records the memory layout of the output of input sections within the final output.
class SectionLayout {
public:
  SectionLayout(u32 output_count, const SectionPlacements &placements);

  // Throws std::out_of_range for an unknown ID.
  const OutputRange &output(OutputSectionID id) const { return _outputs.at(id.value); }
  OutputRange &output(OutputSectionID id) { return _outputs.at(id.value); }
  // Returns the offset of an input section within its output section.
  u64 offset_of(const SectionPlacements &placements, InputElfSectionHandle handle) const;
  // Returns the address of an input section, which is its output section's address plus its offset.
  u64 address_of(const SectionPlacements &placements, InputElfSectionHandle handle) const;
  // Set the offset of the input section at position in placements' selection.
  void set_offset(u32 position, u64 offset) { _offsets.at(position) = offset; }

private:
  // Indexed by OutputSectionID. Index 0 is unused.
  std::vector<OutputRange> _outputs;
  // Indexed like SectionPlacements::sections().
  std::vector<u64> _offsets;
};

// Represents the size and alignment of a placeable input section.
struct SectionExtent {
  u64 size = 0;
  u64 alignment = 1;
};

// Assign addresses to the script's output sections and record offsets for input sections within the output.
SectionLayout lay_out_sections(Diagnostics &diagnostics, const SectionPlacements &placements,
                               const std::function<SectionExtent(InputElfSectionHandle)> &extent_of,
                               const tc::ldscript::Sections &sections);

template <ElfBits B, ElfEndian E>
SectionLayout lay_out_sections(Diagnostics &diagnostics, const bts::PackedInputElfGroup<B, E> &group,
                               const SectionPlacements &placements, const tc::ldscript::Sections &sections) {
  const auto extent_of = [&group](InputElfSectionHandle handle) -> SectionExtent {
    const auto &header = group.file(handle.file).section_headers[handle.index];
    const u64 alignment = bts::word<B>(header.sh_addralign);
    return {bts::word<B>(header.sh_size), alignment == 0 ? 1 : alignment};
  };
  return lay_out_sections(diagnostics, placements, extent_of, sections);
}
} // namespace pepp::link
