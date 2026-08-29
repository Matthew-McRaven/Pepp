/*
 * Copyright (c) 2025-2026 J. Stanley Warford, Matthew McRaven
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
 *
 * Copyright (c) 2024, Alf-André Walla
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * You should have received a copy of the BSD 3-clause license
 * along with this program. If not, see
 * <https://opensource.org/license/bsd-3-clause>
 */
#pragma once
#include <array>
#include <cstddef>
#include <string_view>
#include "core/integers.h"

#define RV32I_LOAD     0b0000011
#define RV32I_STORE    0b0100011
#define RV32I_BRANCH   0b1100011
#define RV32I_JALR     0b1100111
#define RV32I_JAL      0b1101111
#define RV32I_OP_IMM   0b0010011
#define RV32I_OP       0b0110011
#define RV32I_SYSTEM   0b1110011
#define RV32I_LUI      0b0110111
#define RV32I_AUIPC    0b0010111
#define RV32I_FENCE    0b0001111
#define RV64I_OP_IMM32 0b0011011
#define RV64I_OP32     0b0111011
#define RV128I_OP_IMM64 0b1011011
#define RV128I_OP64     0b1111011

#define RV32F_LOAD     0b0000111
#define RV32F_STORE    0b0100111
#define RV32F_FMADD    0b1000011
#define RV32F_FMSUB    0b1000111
#define RV32F_FNMSUB   0b1001011
#define RV32F_FNMADD   0b1001111
#define RV32F_FPFUNC   0b1010011
#define RV32A_ATOMIC   0b0101111

#define RV32F__FADD       0b00000
#define RV32F__FSUB       0b00001
#define RV32F__FMUL       0b00010
#define RV32F__FDIV       0b00011
#define RV32F__FSGNJ_NX   0b00100
#define RV32F__FMIN_MAX   0b00101
#define RV32F__FSQRT      0b01011
#define RV32F__FEQ_LT_LE  0b10100
#define RV32F__FCVT_SD_DS 0b01000
#define RV32F__FCVT_W_SD  0b11000
#define RV32F__FCVT_SD_W  0b11010
#define RV32F__FMV_X_W    0b11100
#define RV32F__FMV_W_X    0b11110
#define RV32V_OP 0b1010111
#define RV32_INSTR_STOP 0x7ff00073

// INVALID must stay 0, and COUNT must be last.
enum class RvOp : u8 {
  INVALID = 0,

  // Upper immediate
  LUI,
  AUIPC,

  // Unconditional jumps
  JAL,
  JALR,

  // Conditional branches
  BEQ,
  BNE,
  BLT,
  BGE,
  BLTU,
  BGEU,

  // Loads
  LB,
  LH,
  LW,
  LBU,
  LHU,

  // Stores
  SB,
  SH,
  SW,

  // Register-immediate ALU
  ADDI,
  SLTI,
  SLTIU,
  XORI,
  ORI,
  ANDI,

  // Register-immediate shifts. On RV32 shamt is 5 bits, so imm[11:5] must be 0000000 for
  // SLLI/SRLI and 0100000 for SRAI; imm[5] set means shamt >= 32 and is reserved.
  SLLI,
  SRLI,
  SRAI,

  // Register-register ALU
  ADD,
  SUB,
  SLL,
  SLT,
  SLTU,
  XOR,
  SRL,
  SRA,
  OR,
  AND,

  // Memory ordering
  FENCE,
  FENCE_TSO,

  // Environment
  ECALL,
  EBREAK,
  // Not an opcode and must be last.
  COUNT
};

namespace riscv {

// Describe the instruction format, which determines which struct should be used to decode instruction bytes.
enum class RvFormat : u8 { Unknown = 0, R, I, S, B, U, J };

// One enumerated constant per format of the instruction string. Each name should lead with the letter of the
// instruction format.
enum class RvSyntax : u8 {
  Unknown = 0,  // INVALID only OR an arbitrary bit-pattern.
  R,            // add rd, rs1, rs2
  I_ALU,        // addi rd, rs1, imm
  I_Shift,      // slli rd, rs1, shamt
  I_Offset,     // lw rd, off(rs1)
  I_Fence,      // fence pred, succ
  I_NoOperands, // ecall, ebreak, fence.tso -- no printed operands, still an I-type word
  S,            // sw rs2, off(rs1)
  B,            // beq rs1, rs2, off
  U,            // lui rd, imm
  J,            // jal rd, off
};


struct RvOpInfo {
  RvOp op;
  std::string_view name;
  RvSyntax syntax;
};

inline constexpr std::array<RvOpInfo, 42> RV_OP_INFO{{
    {RvOp::INVALID, "(invalid)", RvSyntax::Unknown},

    {RvOp::LUI, "lui", RvSyntax::U},
    {RvOp::AUIPC, "auipc", RvSyntax::U},

    {RvOp::JAL, "jal", RvSyntax::J},

    {RvOp::JALR, "jalr", RvSyntax::I_Offset},

    {RvOp::BEQ, "beq", RvSyntax::B},
    {RvOp::BNE, "bne", RvSyntax::B},
    {RvOp::BLT, "blt", RvSyntax::B},
    {RvOp::BGE, "bge", RvSyntax::B},
    {RvOp::BLTU, "bltu", RvSyntax::B},
    {RvOp::BGEU, "bgeu", RvSyntax::B},

    {RvOp::LB, "lb", RvSyntax::I_Offset},
    {RvOp::LH, "lh", RvSyntax::I_Offset},
    {RvOp::LW, "lw", RvSyntax::I_Offset},
    {RvOp::LBU, "lbu", RvSyntax::I_Offset},
    {RvOp::LHU, "lhu", RvSyntax::I_Offset},

    {RvOp::SB, "sb", RvSyntax::S},
    {RvOp::SH, "sh", RvSyntax::S},
    {RvOp::SW, "sw", RvSyntax::S},

    {RvOp::ADDI, "addi", RvSyntax::I_ALU},
    {RvOp::SLTI, "slti", RvSyntax::I_ALU},
    {RvOp::SLTIU, "sltiu", RvSyntax::I_ALU},
    {RvOp::XORI, "xori", RvSyntax::I_ALU},
    {RvOp::ORI, "ori", RvSyntax::I_ALU},
    {RvOp::ANDI, "andi", RvSyntax::I_ALU},

    {RvOp::SLLI, "slli", RvSyntax::I_Shift},
    {RvOp::SRLI, "srli", RvSyntax::I_Shift},
    {RvOp::SRAI, "srai", RvSyntax::I_Shift},

    {RvOp::ADD, "add", RvSyntax::R},
    {RvOp::SUB, "sub", RvSyntax::R},
    {RvOp::SLL, "sll", RvSyntax::R},
    {RvOp::SLT, "slt", RvSyntax::R},
    {RvOp::SLTU, "sltu", RvSyntax::R},
    {RvOp::XOR, "xor", RvSyntax::R},
    {RvOp::SRL, "srl", RvSyntax::R},
    {RvOp::SRA, "sra", RvSyntax::R},
    {RvOp::OR, "or", RvSyntax::R},
    {RvOp::AND, "and", RvSyntax::R},

    {RvOp::FENCE, "fence", RvSyntax::I_Fence},

    {RvOp::FENCE_TSO, "fence.tso", RvSyntax::I_NoOperands},
    {RvOp::ECALL, "ecall", RvSyntax::I_NoOperands},
    {RvOp::EBREAK, "ebreak", RvSyntax::I_NoOperands},
}};

inline constexpr const RvOpInfo &op_info(RvOp op) noexcept {
  const auto i = static_cast<std::size_t>(op);
  return riscv::RV_OP_INFO[i < riscv::RV_OP_INFO.size() ? i : 0];
}
inline constexpr std::string_view mnemonic(RvOp op) noexcept { return op_info(op).name; }
inline constexpr RvSyntax syntax(RvOp op) noexcept { return op_info(op).syntax; }

// Determine the instruction format (for as<> cast) for an opcode or syntax class.
inline constexpr RvFormat format(RvSyntax syn) noexcept {
  switch (syn) {
  case RvSyntax::R: return RvFormat::R;
  case RvSyntax::I_ALU: [[fallthrough]];
  case RvSyntax::I_Shift: [[fallthrough]];
  case RvSyntax::I_Offset: [[fallthrough]];
  case RvSyntax::I_Fence: [[fallthrough]];
  case RvSyntax::I_NoOperands: return RvFormat::I;
  case RvSyntax::S: return RvFormat::S;
  case RvSyntax::B: return RvFormat::B;
  case RvSyntax::U: return RvFormat::U;
  case RvSyntax::J: return RvFormat::J;
  case RvSyntax::Unknown: break;
  }
  return RvFormat::Unknown;
}
inline constexpr RvFormat format(RvOp op) noexcept { return format(syntax(op)); }

// Reverse lookup, so op/syntax/format are all reachable from a mnemonic too. Linear over 42 rows
// and constexpr-evaluable; the assembler keeps its own sorted set for lookups that are hot.
inline constexpr RvOp op_from_name(std::string_view name) noexcept {
  for (std::size_t i = 1; i < riscv::RV_OP_INFO.size(); ++i)
    if (riscv::RV_OP_INFO[i].name == name) return riscv::RV_OP_INFO[i].op;
  return RvOp::INVALID;
}
} // namespace riscv
