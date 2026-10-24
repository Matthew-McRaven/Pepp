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
#include <string_view>
#include <vector>
#include "core/formats/elf/packed_access_headers.hpp"
#include "core/formats/elf/packed_access_symbol.hpp"

namespace pepp::bts {
// Extract the global symbols of an ELF file that contribute to the archive's symbol index.
// Symbols that are defined, but not local or of type STT_FILE or STT_SECTION are extracted.
// string_views are pointers into the elf's string table, so it must outlive the return value.
template <ElfBits B, ElfEndian E> std::vector<std::string_view> defined_globals(const PackedElf<B, E> &elf) {
  std::vector<std::string_view> ret;
  for (u16 sec = 0; sec < elf.section_headers.size(); ++sec) {
    const auto &shdr = elf.section_headers[sec];
    if (shdr.sh_type != bits::to_underlying(SectionTypes::SHT_SYMTAB)) continue;
    // Avoid accessing invalid string table for the symbol table.
    else if (shdr.sh_link >= elf.section_headers.size()) break;
    const PackedSymbolReader<B, E> symtab(elf, sec);
    for (u32 it = 1; it < symtab.symbol_count(); ++it) {
      const auto sym = symtab.get_symbol(it);
      if (sym.st_bind() == SymbolBinding::STB_LOCAL || sym.is_undef()) continue;
      else if (sym.st_type() == SymbolType::STT_FILE || sym.st_type() == SymbolType::STT_SECTION) continue;
      const auto name = symtab.get_symbol_name(it);
      ret.emplace_back(name.data(), name.size());
    }
    // An object file has at most one SHT_SYMTAB.
    break;
  }
  return ret;
}

std::vector<std::string_view> defined_globals(const AnyInputElf &elf);
} // namespace pepp::bts
