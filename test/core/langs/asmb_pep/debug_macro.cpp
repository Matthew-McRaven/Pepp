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

#include <catch.hpp>
#include "core/langs/asmb/debug_line.hpp"
#include "core/langs/asmb_driver.hpp"

TEST_CASE("Pepp ASM .debug_macro", "[scope:core][scope:core.langs][kind:unit][arch:pep10]") {
  using namespace pepp::tc;
  using namespace pepp::bts;
  // Stands in for a book macro, which is not the source's to export.
  auto registry = std::make_shared<MacroRegistry>();
  REQUIRE(registry->insert(std::make_shared<MacroDefinition>(MacroDefinition{"@BOOK", {}, "ASLA"})));
  const auto source = R"(LDWA 1,i
.MACRO twice a
.WORD \a
.WORD \a
.ENDM
.MACRO outer
.MACRO inner
.WORD 1
.ENDM
.ENDM
twice 5
outer
)";
  auto result = assemble_pep10(Pep10DriverConfig{.macros = registry}, FormattingConfig{}, source);
  REQUIRE(result.ok());
  const auto input = to_input_elf(*result.elf());

  auto f = [](const auto &file) {
    constexpr auto E = std::remove_cvref_t<decltype(*file)>::elf_endian;
    const auto macro_index = find_named_section(*file, debugMacroSection);
    REQUIRE(macro_index != 0);
    // The compile unit's only entry is its abbreviation code, then DW_AT_stmt_list, DW_AT_ranges, DW_AT_macros.
    const auto info = section_bytes(*file, find_named_section(*file, debugInfoSection));
    ByteCursor c{info};
    c.offset = sizeof(PackedCompileUnitHeader<E>) + 1 + 4 + 4;
    const u32 macros = c.fixed<U32<E>>();
    REQUIRE(c.ok());
    const auto unit = parse_macro_unit<E>(section_bytes(*file, macro_index), macros);
    REQUIRE(unit.has_value());
    CHECK(unit->entries.size() == 3);
  };
  std::visit(f, input);

  const auto read = std::visit([](const auto &file) { return read_macros(*file); }, input);
  REQUIRE(read.has_value());
  REQUIRE(read->size() == 3);
  const auto &twice = (*read)[0];
  CHECK(twice.line == 2);
  CHECK(twice.definition->name == "twice");
  REQUIRE(twice.definition->arguments.size() == 1);
  CHECK(twice.definition->arguments[0].name == "a");
  CHECK(twice.definition->body == ".WORD \\a\n.WORD \\a");
  CHECK((*read)[1].definition->name == "outer");
  CHECK((*read)[1].line == 6);
  // A definition made while expanding outer takes the line of outer's instantiation.
  CHECK((*read)[2].definition->name == "inner");
  CHECK((*read)[2].line == 12);
  CHECK((*read)[2].definition->body == ".WORD 1");
}
