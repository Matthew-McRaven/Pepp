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

// Returns the output section descriptions of sections, indexed by OutputSectionID. Index 0 is null. Inserting into
// sections.commands invalidates the result.
std::vector<tc::ldscript::OutputSectionDescription *> output_sections(tc::ldscript::Sections &sections);

// Represents the size and alignment of a placeable input section.
struct SectionExtent {
  u64 size = 0;
  u64 alignment = 1;
};

// Assign an address and size to each output section of sections, and an offset to each of its members.
void lay_out_sections(Diagnostics &diagnostics, SectionPlacements &placements,
                      const std::function<SectionExtent(InputElfSectionHandle)> &extent_of,
                      tc::ldscript::Sections &sections);

template <ElfBits B, ElfEndian E>
void lay_out_sections(Diagnostics &diagnostics, const bts::PackedInputElfGroup<B, E> &group,
                      SectionPlacements &placements, tc::ldscript::Sections &sections) {
  const auto extent_of = [&group](InputElfSectionHandle handle) -> SectionExtent {
    const auto &header = group.file(handle.file).section_headers[handle.index];
    const u64 alignment = bts::word<B>(header.sh_addralign);
    return {bts::word<B>(header.sh_size), alignment == 0 ? 1 : alignment};
  };
  lay_out_sections(diagnostics, placements, extent_of, sections);
}
} // namespace pepp::link
