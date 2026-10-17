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

#include "core/langs/asmb/debug_line.hpp"
#include <catch.hpp>
#include "core/formats/elf/packed_access_symbol.hpp"
#include "core/formats/elf/packed_ops.hpp"

TEST_CASE("ET_REL and .debug_line", "[scope:core][scope:core.langs][kind:unit][arch:*]") {
  using namespace pepp::bts;
  using namespace pepp::tc;
  // As in an ET_REL, both sections start at 0. Rows of separate sections share offsets.
  PackedGrowableElfLE32 elf(ElfFileType::ET_REL, ElfMachineType::EM_RISCV, ElfABI::ELFOSABI_NONE);
  ensure_section_header_table(elf);
  const auto text = add_named_section(elf, ".text", SectionTypes::SHT_PROGBITS);
  const auto data = add_named_section(elf, ".data", SectionTypes::SHT_PROGBITS);
  const auto symtab = add_named_symtab(elf, ".symtab", add_named_section(elf, ".strtab", SectionTypes::SHT_STRTAB));
  {
    PackedSymbolWriter<ElfBits::b32, ElfEndian::le> symbols(elf, symtab);
    for (const auto section : {text, data}) {
      PackedElfSymbol<ElfBits::b32, ElfEndian::le> symbol;
      symbol.st_shndx = section;
      symbol.set_type(SymbolType::STT_SECTION);
      symbols.add_symbol(std::move(symbol));
    }
  }
  const auto line = add_named_section(elf, debugLineSection, SectionTypes::SHT_PROGBITS);
  LineProgramWriter<ElfEndian::le> writer(*elf.section_data[line],
                                          pepp::tc::detail::debug_line_header(4, ".", debugLineSourceFile));
  writer.row(0x0, 4, 1, 1);
  writer.end_sequence();
  writer.row(0x0, 4, 1, 5);
  writer.finish();
  const auto &operands = writer.address_operands();
  REQUIRE(operands.size() == 2);
  pepp::tc::detail::write_section_relocations<ElfBits::b32, ElfEndian::le>(
      elf, line, debugLineSection, bits::to_underlying(RelocationsRISCV::R_RISCV_32),
      {{operands[0].offset, text, 0}, {operands[1].offset, data, 0}});

  // text and data are at the same address, but are both kept because they have different sections
  CHECK(read_line_mappings(elf) == std::vector<LineMapping>{{text, 0x0, 1, 0}, {data, 0x0, 5, 0}});
}

TEST_CASE("Macro define strings keep default arguments", "[scope:core][scope:core.langs][kind:unit][arch:*]") {
  using namespace pepp::tc;
  const MacroDefinition macro{
      "m", {{.name = "a"}, {.name = "b", .default_value = "0"}, {.name = "c", .default_value = ""}}, ".WORD \\a"};
  const auto string = detail::define_string(macro);
  CHECK(string == "m(a,b=0,c=) .WORD \\a");
  const auto parsed = detail::parse_define_string(string);
  REQUIRE(parsed.has_value());
  const auto &arguments = (*parsed)->arguments;
  REQUIRE(arguments.size() == 3);
  CHECK(arguments[0].name == "a");
  CHECK(!arguments[0].default_value.has_value());
  CHECK(arguments[1].name == "b");
  CHECK(arguments[1].default_value == "0");
  // An empty default is still a default.
  CHECK(arguments[2].name == "c");
  CHECK(arguments[2].default_value == "");
  CHECK((*parsed)->body == ".WORD \\a");
}
