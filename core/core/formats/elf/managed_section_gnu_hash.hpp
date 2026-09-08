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
#include "core/formats/elf/managed_section.hpp"
#include "core/formats/elf/managed_section_symtab.hpp"

namespace pepp::bts {

/*
 * Represents a SHT_GNU_HASH section, which is used to quickly search a symbol table without a linear scan. This class
 * delegates to PackedGNUHashedSymbolWriter, and as such requires that the associated symtab and strtab be serialized
 * first.
 */
class ManagedGnuHash : public ManagedPayload {
public:
  ManagedGnuHash(SectionRef symtab, SectionRef strtab, GnuHashParameters parameters);

  SectionRef symtab() const noexcept { return _symtab; }
  const GnuHashParameters &parameters() const noexcept { return _parameters; }
  void set_parameters(GnuHashParameters parameters) noexcept { _parameters = parameters; }

  uxword file_bytes(ElfBits bits) const override;
  void collect_dependencies(std::vector<SectionRef> &out) const override;

private:
  SectionRef _symtab, _strtab;
  GnuHashParameters _parameters;
};

} // namespace pepp::bts
