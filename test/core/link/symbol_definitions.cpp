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
#include "core/link/symbol_definitions.hpp"
#include <catch.hpp>
#include <string_view>
#include <vector>
#include "./helpers.hpp"
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/ldscript/parser.hpp"
#include "core/link/section_layout.hpp"

TEST_CASE("symbol definitions", "[scope:core][scope:core.link][kind:unit][arch:*]") {
  using namespace link_test;
  using enum SectionFlags;
  using enum SectionTypes;
  using namespace bits;
  PackedInputElfGroupLE32 group;
  // Section indices start at 2, after the null section and .shstrtab.
  group.add(object_with({{".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR, 3},
                         {".none", SHT_PROGBITS, SHF_ALLOC},
                         {".gone", SHT_PROGBITS, SHF_ALLOC, 1}},
                        {{"main", 2, 1},
                         {"none", 3},
                         {"dead", 4},
                         {"fixed", to_underlying(SectionIndices::SHN_ABS), 0x55}}))
      .value();
  const std::vector<Liveness> files(group.size() + 1, Liveness::Live);
  Diagnostics diagnostics;
  const auto inputs = gather_symbols(diagnostics, group);
  REQUIRE(inputs);
  const auto symbols = resolve_symbols(diagnostics, *inputs, files);

  pepp::tc::DiagnosticTable diag;
  auto script = pepp::tc::ldscript::parse(diag, R"(SECTIONS {
  .text 0x100 : { *(.text) }
  .none : { *(.none) }
  /DISCARD/ : { *(.gone) }
})");
  REQUIRE(diag.count() == 0);
  auto &sections = pepp::tc::ldscript::merge_sections(script);
  auto placements = place_sections(group, files, sections);
  lay_out_sections(diagnostics, group, placements, sections);
  const auto outputs = output_sections(sections);
  const LinkView<ElfBits::b32, ElfEndian::le> link{group, files, *inputs, symbols, placements, outputs};
  const auto definitions = resolved_symbol_definitions(link);
  const auto of = [&](std::string_view name) { return definitions.at(symbols.find(name).value); };

  // A symbol's address is its output section's address + its section's offset + its value.
  CHECK(of("main").section == OutputSectionID{1});
  CHECK(of("main").address == 0x101);
  // Empty sections not included in output but retain addresses for symbols.
  CHECK(of("none").address == 0x103);
  CHECK(of("none"));
  CHECK_FALSE(of("dead"));
  CHECK(of("fixed").absolute);
  CHECK(of("fixed").address == 0x55);
  CHECK_FALSE(of("fixed").section);
}
