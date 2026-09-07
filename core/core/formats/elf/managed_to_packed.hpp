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
#include <map>
#include <span>
#include "core/formats/elf/managed_types.hpp"
#include "core/formats/elf/packed_elf.hpp"

namespace pepp::bts {

class ManagedElf;

struct PackResult {
  AnyGrowableElf elf; // Instantied with the correct bitness/endianness.
  // SectionRefs are not ELF's section indices. While packing, we have to construct a mapping from our opaque type to
  // ELF's real indices. This mapping cannot be reconstructed (without rebuilding the ELF anyway), so we return it.
  std::map<SectionRef, u16> section_indices;
};

/*
 * Make the more abstract ManagedElf into the more concrete PackedElf so that we can take advantage of the serialization
 * machinery around PackedElf. If a value doesn't fit within the target would size, throws rather than truncates.
 *
 * It always orders sections by address, which is what ELF requires inside segments.
 */
PackResult pack(const ManagedElf &elf, std::span<const SectionRef> live);
} // namespace pepp::bts