/*
 * /Copyright (c) 2026. Stanley Warford, Matthew McRaven
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
#include <array>
#include <catch.hpp>
#include <cstdint>
#include <string_view>
#include "core/arch/riscv/isa/rv_instruction.hpp"

namespace {
struct Case {
  uint32_t word;
  std::string_view text;
};

// One entry per current formatting body (2026-08-28), picking the portions most likely to break.
constexpr std::array<Case, 18> CASES{{
    {0x00C58533u, "add x10, x11, x12"},        // r_type
    {0xFEC58513u, "addi x10, x11, -20"},       // i_alu, negative immediate is sign-extended
    {0x41F5D513u, "srai x10, x11, 31"},        // i_shift, maximum RV32 shamt, alternate funct7
    {0xFFC5A503u, "lw x10, -4(x11)"},          // i_offset as a load, negative displacement
    {0x008280E7u, "jalr x1, 8(x5)"},           // i_offset as a jump: register-indirect, not PC-relative
    {0x00C5A823u, "sw x12, 16(x11)"},          // s_store, split immediate reassembled
    {0xFEC58CE3u, "beq x11, x12, -8"},         // b_branch, negative PC-relative offset
    {0x7EC5EFE3u, "bltu x11, x12, 4094"},      // b_branch, largest positive offset
    {0xFFFFF56Fu, "jal x10, -2"},              // j_jal, negative offset across all four fields
    {0xFFFFF537u, "lui x10, 0xfffff"},         // u_upper, raw 20-bit field in hex
    {0x00001097u, "auipc x1, 0x1"},            // u_upper, small value keeps its 0x prefix
    {0x0330000Fu, "fence rw, rw"},             // fence, pred and succ decoded to iorw letters
    {0x0FF0000Fu, "fence iorw, iorw"},         // fence, every ordering bit set
    {0x0000000Fu, "fence 0, 0"},               // fence ordering nothing prints 0, not empty
    {0x8330000Fu, "fence.tso"},                // no_operands
    {0x00000073u, "ecall"},                    // no_operands
    {0x00100073u, "ebreak"},                   // no_operands, distinguished from ecall by imm
    {0x00001067u, "unknown 0x00001067"},       // reserved: JALR requires funct3 == 0
}};
} // namespace

TEST_CASE("RISC-V ISA disassembly", "[scope:core][scope:core.arch][kind:unit][arch:riscv]") {
  SECTION("one instruction per formatting body") {
    for (const auto &c : CASES) {
      CAPTURE(c.word);
      CHECK(riscv::rv_instruction2(c.word).to_string() == c.text);
    }
  }

  SECTION("encodings RV32I are marked as unknown") {
    // RV64-only load width, Zicsr, and a compressed encoding: all outside the base set.
    CHECK(riscv::rv_instruction2(0x0005B503u).to_string() == "unknown 0x0005b503"); // ld
    CHECK(riscv::rv_instruction2(0x30551073u).to_string() == "unknown 0x30551073"); // csrrw
    CHECK(riscv::rv_instruction2(0x00004081u).to_string() == "unknown 0x4081");     // 16-bit
  }

  SECTION("shamt above 31 is reserved on RV32") {
    // imm[5] set means a shift of 32 or more, which RV64 encodes but RV32 leaves reserved.
    CHECK(riscv::rv_instruction2(0x0205D513u).to_string() == "unknown 0x0205d513");
  }
}
