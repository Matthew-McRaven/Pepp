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
#include <span>
#include <vector>
#include "core/formats/elf/managed_section.hpp"
#include "core/formats/elf/managed_section_symtab.hpp"

namespace pepp::bts {

struct ManagedRelocation {
  uxword offset = 0;
  ManagedSymbolTable::entry_ptr_t symbol;
  // Machine-specific; see Relocations* in enums.hpp.
  u32 type = 0;
  sxword addend = 0;
};

/*
 * Represents a SHT_RELA section, which provides relocations against a single section.
 * I preferred relocations with addends (RELA) rather than REL simply because RV32 uses RELA in practice.
 *
 * Carries no dependencies
 */
class ManagedRelocTable : public ManagedPayload {
public:
  void add(ManagedRelocation entry);
  std::span<const ManagedRelocation> entries() const noexcept { return _entries; }

  uxword file_bytes(ElfBits bits) const override;

private:
  std::vector<ManagedRelocation> _entries;
};

class ManagedElf;

/*
 * Create a RELA table for a given section, naming it `.rela.<target>`. If the relocation section does not exist, it
 * will be created. If it already exists, it will be returned as-is.
 */
ManagedRelocTable &relocations_for(ManagedElf &elf, SectionRef target, SectionRef symtab);

} // namespace pepp::bts
