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
#include "core/formats/elf/packed_access_symbol.hpp"
#include "core/formats/elf/packed_input_group.hpp"

namespace pepp::link {
// A pointer to one symbol in an input file. The index is an into into the original symbol table's SYMTAB.
// Name points into the string table.
struct FileSymbol {
  u32 index = 0;
  std::string_view name;
  bool operator==(const FileSymbol &) const = default;
};

// The non-local symbols of one input file, in symbol table order.
struct FileSymbols {
  // Defined, other than STT_FILE and STT_SECTION. Includes weak and common symbols.
  std::vector<FileSymbol> globals;
  std::vector<FileSymbol> undefined;
};

// Extract all non-local symbols for each ELF file in the group. Index 0 is empty because ID 0 is invalid. Files without
// symbol tables are empty. Archive's symbol index is ignored, so members' symbol tables are re-read from their ELF
// symtabs.
template <bts::ElfBits B, bts::ElfEndian E>
std::vector<FileSymbols> collect_symbols(const bts::PackedInputElfGroup<B, E> &group) {
  using namespace bts;
  std::vector<FileSymbols> ret(group.size() + 1);
  for (u32 id = 1; id <= group.size(); ++id) {
    const auto &elf = group.file(ElfFileID(id));
    for (u16 sec = 0; sec < elf.section_headers.size(); ++sec) {
      const auto &shdr = elf.section_headers[sec];
      if (shdr.sh_type != bits::to_underlying(SectionTypes::SHT_SYMTAB)) continue;
      // Avoid reading through an invalid string table.
      else if (shdr.sh_link >= elf.section_headers.size()) break;
      const PackedSymbolReader<B, E> symtab(elf, sec);
      // Index 0 is the reserved null symbol.
      for (u32 it = 1; it < symtab.symbol_count(); ++it) {
        const auto sym = symtab.get_symbol(it);
        if (sym.st_bind() == SymbolBinding::STB_LOCAL) continue;
        const auto span = symtab.get_symbol_name(it);
        const FileSymbol symbol{it, std::string_view(span.data(), span.size())};
        if (sym.is_undef()) ret[id].undefined.push_back(symbol);
        else if (sym.st_type() != SymbolType::STT_FILE && sym.st_type() != SymbolType::STT_SECTION)
          ret[id].globals.push_back(symbol);
      }
      // An object file has at most one SHT_SYMTAB.
      break;
    }
  }
  return ret;
}
} // namespace pepp::link
