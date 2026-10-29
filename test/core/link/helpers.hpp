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
#include <catch.hpp>
#include <fstream>
#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include "core/formats/ar/packed_archive.hpp"
#include "core/formats/elf/packed_io.hpp"
#include "core/formats/elf/packed_ops.hpp"
#include "core/link/symbol_resolution.hpp"

namespace link_test {
using namespace pepp::bts;
using namespace pepp::link;

struct TestSymbol {
  std::string_view name;
  bool defined;
  SymbolBinding bind = SymbolBinding::STB_GLOBAL;
  SymbolType type = SymbolType::STT_NOTYPE;
  SymbolVisibility visibility = SymbolVisibility::STV_DEFAULT;
  // Defined in SHN_COMMON rather than .text.
  bool common = false;
  u32 size = 0;
  // A common symbol's alignment.
  u32 value = 0;
};
using Symbols = std::initializer_list<TestSymbol>;

// An object with .text, and a symbol table holding symbols unless it is empty.
template <ElfBits B = ElfBits::b32, ElfEndian E = ElfEndian::le> PackedGrowableElfFile<B, E> elf_of(Symbols symbols) {
  PackedGrowableElfFile<B, E> elf(ElfFileType::ET_REL, ElfMachineType::EM_PEP10, ElfABI::ELFOSABI_NONE);
  ensure_section_header_table(elf);
  const auto text = add_named_section(elf, ".text", SectionTypes::SHT_PROGBITS);
  if (symbols.size() == 0) return elf;
  const auto symtab = add_named_symtab(elf, ".symtab", add_named_section(elf, ".strtab", SectionTypes::SHT_STRTAB));
  PackedSymbolWriter<B, E> writer(elf, symtab);
  for (const auto &symbol : symbols) {
    PackedElfSymbol<B, E> sym;
    sym.set_bind(symbol.bind);
    sym.set_type(symbol.type);
    sym.set_visibility(symbol.visibility);
    sym.st_size = symbol.size;
    sym.st_value = symbol.value;
    const u16 section = symbol.common ? bits::to_underlying(SectionIndices::SHN_COMMON) : symbol.defined ? text : 0;
    writer.add_symbol(std::move(sym), symbol.name, section);
  }
  writer.arrange_local_symbols();
  return elf;
}
template <ElfBits B = ElfBits::b32, ElfEndian E = ElfEndian::le> AnyInputElf object_of(Symbols symbols) {
  auto elf = elf_of<B, E>(symbols);
  return to_input_elf(elf);
}
inline std::vector<u8> bytes_of(Symbols symbols) {
  auto elf = elf_of(symbols);
  return elf_bytes(elf);
}

inline void write_bytes(const std::string &fname, const std::vector<u8> &bytes) {
  std::ofstream(fname, std::ios::binary).write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
}

// An archive with one member for each entry of members. Names are 0.o, 1.o, ...
inline PackedGrowableArchive archive_of(std::initializer_list<Symbols> members) {
  PackedGrowableArchive ret;
  for (const auto &symbols : members) {
    const auto bytes = bytes_of(symbols);
    const auto name = std::to_string(ret.member_count()) + ".o";
    REQUIRE(ret.add_member(name, std::make_shared<BlockStorage>(std::vector<char>(bytes.begin(), bytes.end()))));
  }
  return ret;
}
inline std::vector<u8> archive_bytes_of(std::initializer_list<Symbols> members) {
  auto archive = archive_of(members);
  return archive_bytes(archive).value();
}

inline std::vector<Diagnostic> entries_of(const Diagnostics &diagnostics) {
  return {diagnostics.begin(), diagnostics.end()};
}

// Looks up the entry of a file's symbol index or fail the test.
inline auto entry_of(const InputSymbols &table) {
  return [&table](ElfFileID file, u32 index) {
    const auto id = table.find(file, index);
    REQUIRE(id);
    return *id;
  };
}
} // namespace link_test
