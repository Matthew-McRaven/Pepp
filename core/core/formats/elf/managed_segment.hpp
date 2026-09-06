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
#include <vector>
#include "core/formats/elf/enums.hpp"
#include "core/formats/elf/managed_types.hpp"

namespace pepp::bts {

/*
 * While ELF specification describes segments in terms of file offsets and sizes, this abstraction considers segments to
 * be a collection of sections. This makes it impossible to create all possible valid ELF files. However, the
 * compiler/assembler no longer needs to think in terms of asbolute file offsets and sizes, which is convenient for
 * incrementally producing output. The Packed* classes can give you finer control over file and memory layout.
 * This decision allows us to remove p_offset, p_filesz, and p_memsz from this class and only compute them as needed.
 */
class ManagedSegment {
public:
  ManagedSegment() = default;
  ManagedSegment(SegmentType type, SegmentFlags flags) : type(type), flags(flags) {}

  SegmentType type = SegmentType::PT_NULL;
  SegmentFlags flags = {};
  // Values 0 and 1 indicate no alignment and must pe a power-of-two per Figure 2-1 after ELF TIS v1.2
  // This only constrains the file offset and memory alignment of the start of the segment. Contained sections may
  // require higher alignment within this contiguous segment.
  u32 align = 1;
  uxword vaddr = 0, paddr = 0;

  // Ordered list of sections which belong to this Segment. It is possible to construct mutually impossible segments,
  // e.g., Seg1==[A,B], and Seg2=[B,A]. This is checked at serialization time rather than as you author the Segments.
  std::vector<SectionRef> sections;
};

} // namespace pepp::bts
