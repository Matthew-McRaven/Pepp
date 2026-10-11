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
#include <vector>
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/asmb_riscv/codegen.hpp"
#include "core/langs/asmb_riscv/parser.hpp"

namespace {
using pepp::tc::ArgumentConstraint;
using enum pepp::tc::expr::Signedness;
auto data = [](auto str) { return pepp::tc::support::SeekableData{str}; };
} // namespace

TEST_CASE("Argument constraints of RISC-V lines", "[scope:core][scope:core.langs][kind:unit][arch:*]") {
  pepp::tc::DiagnosticTable diag;
  auto p = pepp::tc::parser::RISCVParser(data(".byte 1\n.half 1\n.word 1\naddi x1, x1, 1\nsw x1, 0(x2)\n"
                                              "slli x1, x1, 1\nlui x1, 1\nbeq x1, x2, 0\njal x1, 0\nadd x1, x2, x3"));
  const auto lines = p.parse(diag);
  REQUIRE(diag.count() == 0);
  REQUIRE(lines.size() == 10);
  const std::vector<std::optional<ArgumentConstraint>> expected{
      ArgumentConstraint{8, Bits},
      ArgumentConstraint{16, Bits},
      ArgumentConstraint{32, Bits},
      ArgumentConstraint{12, Signed},
      ArgumentConstraint{12, Signed},
      ArgumentConstraint{5, Unsigned},
      ArgumentConstraint{20, Unsigned},
      ArgumentConstraint{13, Signed, 2},
      ArgumentConstraint{21, Signed, 2},
      std::nullopt};
  for (std::size_t it = 0; it < lines.size(); it++) {
    CAPTURE(it);
    CHECK(pepp::tc::riscv_argument_constraint(*lines[it]) == expected[it]);
  }
}
