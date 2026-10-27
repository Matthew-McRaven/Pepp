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

#include "core/link/file_symbols.hpp"
#include <catch.hpp>
#include <string_view>
#include <vector>
#include "core/formats/ar/packed_archive.hpp"
#include "core/formats/elf/packed_io.hpp"
#include "core/formats/elf/packed_ops.hpp"

namespace {
using namespace pepp::bts;
using namespace pepp::link;
using Elf = PackedGrowableElfFile<ElfBits::b32, ElfEndian::le>;

// An object with .text and an optional symbol table with every distinguishable binding.
Elf object(bool with_symbols) {
  Elf elf(ElfFileType::ET_REL, ElfMachineType::EM_PEP10, ElfABI::ELFOSABI_NONE);
  ensure_section_header_table(elf);
  const auto text = add_named_section(elf, ".text", SectionTypes::SHT_PROGBITS);
  if (!with_symbols) return elf;
  const auto symtab = add_named_symtab(elf, ".symtab", add_named_section(elf, ".strtab", SectionTypes::SHT_STRTAB));
  PackedSymbolWriter<ElfBits::b32, ElfEndian::le> writer(elf, symtab);
  const auto symbol = [](SymbolBinding bind, SymbolType type) {
    PackedElfSymbol<ElfBits::b32, ElfEndian::le> ret;
    ret.set_bind(bind);
    ret.set_type(type);
    return ret;
  };
  writer.add_symbol(symbol(SymbolBinding::STB_LOCAL, SymbolType::STT_SECTION), "", text); // 1
  writer.add_symbol(symbol(SymbolBinding::STB_LOCAL, SymbolType::STT_NOTYPE), "loop", text);
  writer.add_symbol(symbol(SymbolBinding::STB_GLOBAL, SymbolType::STT_FUNC), "main", text); // 3
  writer.add_symbol(symbol(SymbolBinding::STB_WEAK, SymbolType::STT_FUNC), "helper", text);
  writer.add_symbol(symbol(SymbolBinding::STB_GLOBAL, SymbolType::STT_NOTYPE), "printf", 0); // 5
  writer.add_symbol(symbol(SymbolBinding::STB_WEAK, SymbolType::STT_NOTYPE), "optional", 0);
  return elf;
}
} // namespace

TEST_CASE("Collect globals / undefineds", "[scope:core][scope:core.link][kind:unit][arch:*]") {
  PackedInputElfGroupLE32 group;
  auto with = object(true), without = object(false);
  const auto a = group.add(to_input_elf(with)).value(), b = group.add(to_input_elf(without)).value();

  SECTION("Weak, global, and undefined symbols") {
    const auto symbols = collect_symbols(group);
    REQUIRE(symbols.size() == 3);
    CHECK(symbols[a.value].globals == std::vector<FileSymbol>{{3, "main"}, {4, "helper"}});
    CHECK(symbols[a.value].undefined == std::vector<FileSymbol>{{5, "printf"}, {6, "optional"}});
    // A file without a symbol table has no symbols.
    CHECK(symbols[b.value].globals.empty());
    CHECK(symbols[b.value].undefined.empty());
  }
  SECTION("Archive symbol indices are ignored") {
    PackedGrowableArchive archive;
    const auto bytes = elf_bytes(with);
    // An incomplete symbol index in the archive, proving that symbols are read via ELF symtab not archive.
    const std::vector<std::string_view> index{"main"};
    REQUIRE(archive.add_member("a.o", std::make_shared<BlockStorage>(std::vector<char>(bytes.begin(), bytes.end())),
                               index));
    auto input = to_input_archive(archive);
    REQUIRE(input);
    REQUIRE(input->symbol_index());
    const auto member = *input->members().begin();
    REQUIRE(member);
    const auto id = group.add(std::move(*input));
    REQUIRE(id);
    const auto c = group.add_member(*id, *member);
    REQUIRE(c);
    const auto symbols = collect_symbols(group);
    // helper is collected anyway, from the member's own symbol table.
    CHECK(symbols[c->value].globals == symbols[a.value].globals);
    CHECK(symbols[c->value].undefined == symbols[a.value].undefined);
  }
}
