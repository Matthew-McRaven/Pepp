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
#include <tuple>
#include <utility>
#include <vector>
#include "core/langs/asmb/codegen_error.hpp"
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/asmb_driver.hpp"
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

TEST_CASE("RISC-V data arguments are validated during codegen", "[scope:core][scope:core.langs][kind:unit][arch:*]") {
  using E = pepp::tc::CodegenError;
  const auto assemble = [](const char *source) {
    return pepp::tc::assemble_riscv(pepp::tc::RISCVDriverConfig{}, pepp::tc::FormattingConfig{}, source);
  };
  for (const char *source : {".byte 255", ".byte -128", ".half 0xFFFF", ".word -1", "l: .word l", ".word undefined"}) {
    CAPTURE(source);
    CHECK(assemble(source).ok());
  }

  using UE = E::UnaryError;
  for (const auto &[source, error, bits] : std::vector<std::tuple<const char *, UE, const char *>>{
           {".byte 256", UE::Argument_ValueDoesNotFit, "8 bits"},
           {".byte 255 + 1", UE::Argument_ValueDoesNotFit, "8 bits"},
           {".byte k\nk: .equ 300", UE::Argument_ValueDoesNotFit, "8 bits"},
           {".half 0x10000", UE::Argument_ValueDoesNotFit, "16 bits"},
           {"l: .half l", UE::Argument_AddressDoesNotFit, "16 bits"},
           {".byte undefined", UE::Argument_AddressDoesNotFit, "8 bits"},
       }) {
    CAPTURE(source);
    const auto result = assemble(source);
    REQUIRE(result.diagnostics.count() == 1);
    CHECK(result.diagnostics.cbegin()->second == E::to_string(error, bits));
  }
}

TEST_CASE("RISC-V instruction arguments are validated at codegen",
          "[scope:core][scope:core.langs][kind:unit][arch:*]") {
  using E = pepp::tc::CodegenError;
  const auto assemble = [](const char *source) {
    return pepp::tc::assemble_riscv(pepp::tc::RISCVDriverConfig{}, pepp::tc::FormattingConfig{}, source);
  };
  // An undefined target is only known once linked.
  for (const char *source : {"addi x1, x1, 2047", "addi x1, x1, -2048", "slli x1, x1, 31", "lui x1, 0xFFFFF",
                             "lui x1, %hi(0x12345678)", "l: beq x1, x2, l", "jal x1, undefined",
                             "l: auipc x1, %pcrel_hi(l)"}) {
    CAPTURE(source);
    CHECK(assemble(source).ok());
  }

  using UE = E::UnaryError;
  for (const auto &[source, error, bits] : std::vector<std::tuple<const char *, UE, const char *>>{
           {"addi x1, x1, 2048", UE::Argument_ValueDoesNotFit, "signed 12 bits"},
           {"slli x1, x1, 32", UE::Argument_ValueDoesNotFit, "unsigned 5 bits"},
           {"lui x1, 0x100000", UE::Argument_ValueDoesNotFit, "unsigned 20 bits"},
           {"beq x1, x2, 4096", UE::Argument_ValueDoesNotFit, "signed 13 bits"},
           // l is treated as an offset (effectively l - .)
           {"beq x1, x2, l\n.skip 4096\nl:", UE::Argument_ValueDoesNotFit, "signed 13 bits"},
           {"l: addi x1, x1, l", UE::Argument_AddressDoesNotFit, "signed 12 bits"},
       }) {
    CAPTURE(source);
    const auto result = assemble(source);
    REQUIRE(result.diagnostics.count() == 1);
    CHECK(result.diagnostics.cbegin()->second == E::to_string(error, bits));
  }
  const auto odd = assemble("beq x1, x2, 3");
  REQUIRE(odd.diagnostics.count() == 1);
  CHECK(odd.diagnostics.cbegin()->second == E::to_string(E::NullaryError::Argument_OddOffset));
}
