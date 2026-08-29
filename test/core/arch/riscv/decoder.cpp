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
#include <stdexcept>
#include "core/arch/riscv/isa/rv_instruction.hpp"
#include "core/arch/riscv/isa/rv_instruction_list.hpp"
#include "core/integers.h"

namespace {
// Instruction decoding operates on three fields the 7-bit opcode, funct3, and inst[31:20]/high12, which carries
// funct7/funct5 where present and the immediate otherwise.
constexpr u32 make_word(u32 opcode, u32 funct3, u32 high12, u32 rd, u32 rs1) {
  return (high12 << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
}
// Only works for the uncompressed 32-bit encodings, whose low two opcode bits are 0b11.
constexpr u32 MAJORS = 32, FUNCT3S = 8, HIGH12S = 4096;
constexpr u32 DECISION_SPACE = MAJORS * FUNCT3S * HIGH12S;
constexpr u32 major_opcode(u32 major) { return (major << 2) | 0b11; }

// Which of the 2^20 patterns an op covers, based on syntax rather than instruction format.
// Use syntax group to shrink the state space. e.g., immediate shifts have fewer legal/useful variants than a normal
// I-type. Counts are relative to the swept fields only.
constexpr u32 claimed(riscv::RvSyntax syntax) {
  switch (syntax) {
  // opcode + funct3 + funct7 fixed, leaving rs2's five bits.
  case riscv::RvSyntax::R: return 32;
  // opcode + funct3 + imm[11:5] fixed, leaving shamt's five bits.
  case riscv::RvSyntax::I_Shift: return 32;
  // opcode + funct3 fixed, leaving whole immediate free.
  case riscv::RvSyntax::I_ALU:
  case riscv::RvSyntax::I_Offset:
  case riscv::RvSyntax::S:
  case riscv::RvSyntax::B: return HIGH12S;
  // only opcode is fixed.
  case riscv::RvSyntax::U:
  case riscv::RvSyntax::J: return FUNCT3S * HIGH12S;
  // Every immediate is available except the one reserved by FENCE.TSO
  case riscv::RvSyntax::I_Fence: return HIGH12S - 1;
  // Only one valid immediate for that instruction.
  case riscv::RvSyntax::I_NoOperands: return 1;
  case riscv::RvSyntax::Unknown: break;
  default: throw std::logic_error("Invalid syntax");
  }
  return 0;
}
} // namespace

TEST_CASE("RISC-V decoder covers its decision space", "[scope:core][scope:core.arch][kind:unit][arch:riscv]") {
  SECTION("every op claims exactly its syntax's share, and from those three fields alone") {
    struct Pattern {
      u32 rd, rs1;
    };
    // Sweep the deciding fields exhaustively, but sample the register fields to avoid a full 2^32
    // iterations.
    static constexpr std::array<Pattern, 2> PATTERNS{{{0b11111, 0b11111}, {0b10101, 0b01010}}};

    // Allocate an array at least as large as the size of the opcode type. Checking if there are
    // any opcodes beyond the RvOp::COUNT position helps validate decoding correctness.
    std::array<u32, 1 << (8 * sizeof(RvOp))> seen{};
    // bad_* only samples the first mismatch, but at least it gives you a starting point to debug.
    u32 mismatches = 0, bad_opcode = 0, bad_funct3 = 0, bad_high12 = 0;
    for (u32 major = 0; major < MAJORS; ++major)
      for (u32 funct3 = 0; funct3 < FUNCT3S; ++funct3)
        for (u32 high12 = 0; high12 < HIGH12S; ++high12) {
          const auto opcode = major_opcode(major);
          const auto expected = riscv::decode(riscv::rv_instruction2{make_word(opcode, funct3, high12, 0, 0)});
          // Only increase count for a single rs1/rd pattern, since the other pairs are expected to decoder identically.
          ++seen[static_cast<uint8_t>(expected)];
          // Changing rd/rs1 must not change the decoded op. While ecall, ebreak and fence  require
          // particular values in those fields, enforcement is at execution time rather than decode.
          for (const auto &pat : PATTERNS) {
            const auto inst = riscv::rv_instruction2{make_word(opcode, funct3, high12, pat.rd, pat.rs1)};
            if (riscv::decode(inst) != expected && mismatches++ == 0)
              bad_opcode = opcode, bad_funct3 = funct3, bad_high12 = high12;
          }
        }

    CAPTURE(mismatches, bad_opcode, bad_funct3, bad_high12);
    CHECK(mismatches == 0);

    u32 valid = 0;
    for (std::size_t i = 1; i < static_cast<std::size_t>(RvOp::COUNT); ++i) {
      const auto op = static_cast<RvOp>(i);
      CAPTURE(riscv::mnemonic(op));
      // Too few implies unreachable encoding. Too many and this op stole another's encoding(s).
      CHECK(seen[i] == claimed(riscv::syntax(op)));
      valid += seen[i];
    }
    // Patterns not claimed by an op are invalid.
    CHECK(seen[static_cast<std::size_t>(RvOp::INVALID)] == DECISION_SPACE - valid);
    // Nothing may decode at or beyond COUNT, which would mean we returned a value outside the enum.
    for (std::size_t i = static_cast<std::size_t>(RvOp::COUNT); i < seen.size(); ++i) {
      CAPTURE(i);
      CHECK(seen[i] == 0);
    }
  }

  SECTION("encodings beyond implemented extensions are marked invalid") {
    struct Case {
      u32 word;
      const char *why;
    };
    static constexpr std::array<Case, 9> RESERVED{{
        {0x00001067u, "JALR requires funct3 == 0"},
        {0x00002063u, "BRANCH funct3 010 is reserved"},
        {0x00003063u, "BRANCH funct3 011 is reserved"},
        {0x0005B503u, "LD is an RV64 load width"},
        // A shamt of 32 needs inst[25], which is the low bit of funct7. An oversized shift reads as funct7 0100001 for
        // SRAI and 0000001 for SRLI.
        {0x4205D513u, "SRAI with shamt >= 32 is an RV64 shift width"},
        {0x0205D513u, "SRLI with shamt >= 32 is an RV64 shift width"},
        {0x02C58533u, "MUL belongs to the M extension"},
        {0x30551073u, "CSRRW belongs to Zicsr"},
        {0x00004081u, "compressed encodings are the C extension"},
    }};
    for (const auto &c : RESERVED) {
      CAPTURE(c.word, c.why);
      CHECK(riscv::decode(riscv::rv_instruction2{c.word}) == RvOp::INVALID);
    }
  }
}
