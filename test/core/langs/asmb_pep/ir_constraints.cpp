/*
 * Copyright (c) 2026. Stanley Warford, Matthew McRaven
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <catch.hpp>
#include <optional>
#include <utility>
#include <vector>
#include "core/compile/macro/macro_registry.hpp"
#include "core/langs/asmb/codegen_error.hpp"
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/asmb_driver.hpp"
#include "core/langs/asmb_pep/codegen.hpp"
#include "core/langs/asmb_pep/parser.hpp"

namespace {
using pepp::tc::ArgumentConstraint;
using enum pepp::tc::expr::Signedness;
auto data = [](auto str) { return pepp::tc::support::SeekableData{str}; };
} // namespace

TEST_CASE("Argument constraints of Pep/10 lines", "[scope:core][scope:core.langs][kind:unit][arch:*]") {
  pepp::tc::DiagnosticTable diag;
  auto p = pepp::tc::parser::PepParser(data(".BYTE 1\n.WORD 1\nLDWA 1,i\nASLA\n.ASCII \"a\""),
                                       std::make_shared<pepp::tc::MacroRegistry>());
  const auto lines = p.parse(diag);
  REQUIRE(diag.count() == 0);
  REQUIRE(lines.size() == 5);
  const std::vector<std::optional<ArgumentConstraint>> expected{
      ArgumentConstraint{8, Bits},
      ArgumentConstraint{16, Bits},
      ArgumentConstraint{16, Bits},
      std::nullopt,
      std::nullopt};
  for (std::size_t it = 0; it < lines.size(); it++) {
    CAPTURE(it);
    CHECK(pepp::tc::pepp_argument_constraint(*lines[it]) == expected[it]);
  }
}

TEST_CASE("Pep/10 arguments are validated at codegen", "[scope:core][scope:core.langs][kind:unit][arch:*]") {
  using E = pepp::tc::CodegenError;
  const auto assemble = [](const char *source) {
    return pepp::tc::assemble_pep10(pepp::tc::Pep10DriverConfig{}, pepp::tc::FormattingConfig{}, source);
  };
  for (const char *source : {".BYTE 255", ".BYTE -128", ".WORD 0xFFFF", ".WORD -1", "LDWA 0xFFFF,i", "l: .BYTE l"}) {
    CAPTURE(source);
    CHECK(assemble(source).ok());
  }

  // Value of k is not known when .byte is parsed, which demonstrates why we check constraints during codegen.
  for (const auto &[source, bits] :
       std::vector<std::pair<const char *, const char *>>{{".BYTE 256", "8 bits"},
                                                          {".BYTE -129", "8 bits"},
                                                          {".BYTE k\nk: .EQUATE 300", "8 bits"},
                                                          {".WORD 70000", "16 bits"},
                                                          {"LDWA 0x10000,i", "16 bits"}}) {
    CAPTURE(source);
    const auto result = assemble(source);
    REQUIRE(result.diagnostics.count() == 1);
    CHECK(result.diagnostics.cbegin()->second.message == E::to_string(E::UnaryError::Argument_ValueDoesNotFit, bits));
    CHECK(result.diagnostics.cbegin()->second.severity == pepp::tc::Severity::Error);
  }
}
