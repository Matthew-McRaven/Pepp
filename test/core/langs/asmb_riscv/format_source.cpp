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
#include "core/langs/asmb_riscv/text_format.hpp"
#include <array>
#include <catch.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <string>
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/asmb_riscv/parser.hpp"

namespace {
static auto data = [](auto str) { return pepp::tc::support::SeekableData{std::string(str)}; };

// Parse one line and format it. Failure to parse terminates test.
std::string parse_format(const std::string &source) {
  pepp::tc::DiagnosticTable diag;
  auto parser = pepp::tc::parser::RISCVParser(data(source));
  auto lines = parser.parse(diag);
  CAPTURE(source);
  REQUIRE(diag.count() == 0);
  REQUIRE(lines.size() == 1);
  return pepp::tc::riscv_format_source(lines[0].get());
}

struct Case {
  const char *source;
  const char *expected;
};

static const std::array<Case, 27> CASES{{
    {"add x1, x2, x3", "         add     x1, x2, x3"},                  // R: rd, rs1, rs2
    {"addi x1, x2, -12", "         addi    x1, x2, -12"},               // I_ALU, negative decimal
    {"slli x1, x2, 5", "         slli    x1, x2, 5"},                   // I_Shift: shamt
    {"lw x1, 8(x2)", "         lw      x1, 8(x2)"},                     // I_Offset: rd, off(rs1)
    {"sw x3, 16(x2)", "         sw      x3, 16(x2)"},                   // S: rs2, off(rs1)
    {"jalr x1, 0(x2)", "         jalr    x1, 0(x2)"},                   // parenthesised, non-load opcode
    {"beq x1, x2, 8", "         beq     x1, x2, 8"},                    // B: rs1, rs2, off
    {"bgt x1, x2, 8", "         bgt     x1, x2, 8"},                    // reversed pseudo: declares rs2 first
    {"beqz x1, 8", "         beqz    x1, 8"},                           // rs1, off; rs2 pinned
    {"blez x1, 8", "         blez    x1, 8"},                           // rs2, off; rs1 pinned
    {"lui x5, 0x10", "         lui     x5, 0x00000010"},                // U: hex renders zero-padded
    {"jal x1, 16", "         jal     x1, 16"},                          // J, two-operand spelling
    {"jal 16", "         jal     16"},                                  // J, one-operand spelling
    {"jr x1", "         jr      x1"},                                   // lone source register
    {"mv x1, x2", "         mv      x1, x2"},                           // rd, rs
    {"seqz x1, x2", "         seqz    x1, x2"},                         // rd, rs1
    {"snez x1, x2", "         snez    x1, x2"},                         // rd, rs2
    {"fence rw, wr", "         fence   rw, rw"},                        // orderings unpacked from the immediate
    {"fence w, 0", "         fence   w, 0"},                            // empty ordering prints as 0
    {"ecall", "         ecall"},                                        // no operands
    {"ret", "         ret"},                                            // no operands
    {"beq x1, x2, target", "         beq     x1, x2, target"},          // symbolic immediate
    {"foo: add x1, x2, x3", "foo:     add     x1, x2, x3"},             // symbol declaration
    {"add x1, x2, x3 # hi", "         add     x1, x2, x3        # hi"}, // trailing comment
    {".word 5", "         .word   5"},                                  // literal directive
    {".byte 1", "         .byte   1"},                                  // literal directive
    {"bar: .equ 5", "bar:     .equ    5"},                              // equate directive
}};
} // namespace

TEST_CASE("RISCV ASM source formatting",
          "[scope:core][scope:core.langs][level:asmb3][level:asmb5][kind:unit][arch:riscv]") {
  SECTION("a line formats back to its canonical spelling") {
    // Source on the left, the text it must format to on the right. Columns are 9/8/18 wide, and
    // trailing space is trimmed, so a line with no comment ends at its last operand.
    // Assemble+format twice to ensure that formatting is stable.
    for (const auto &c : CASES) {
      CAPTURE(c.source);
      auto actual = parse_format(c.source);
      auto reformatted = parse_format(actual);
      CHECK(actual == c.expected);
      CHECK(reformatted == c.expected);
    }
  }
}
