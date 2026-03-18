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

#pragma once
#include "core/arch/riscv/isa/rv_instruction_list.hpp"
#include "core/arch/riscv/isa/rvi.hpp"

namespace riscv {
const auto LUI = U::pattern{.opcode = RV32I_LUI};
const auto AUIPC = U::pattern{.opcode = RV32I_AUIPC};
// Branches
const auto JAL = J::pattern{.opcode = RV32I_JAL};
// J is JAL with rd=x0
const auto JALR = I::pattern{.opcode = RV32I_JALR, .funct3 = 0b000};
const auto BEQ = B::pattern{.opcode = RV32I_BRANCH, .funct3 = 0b000};
const auto BNE = B::pattern{.opcode = RV32I_BRANCH, .funct3 = 0b001};
const auto BLT = B::pattern{.opcode = RV32I_BRANCH, .funct3 = 0b100};
const auto BGE = B::pattern{.opcode = RV32I_BRANCH, .funct3 = 0b101};
const auto BLTU = B::pattern{.opcode = RV32I_BRANCH, .funct3 = 0b110};
const auto BGEU = B::pattern{.opcode = RV32I_BRANCH, .funct3 = 0b111};
// Loads
const auto LB = I::pattern{.opcode = RV32I_LOAD, .funct3 = 0b000};
const auto LH = I::pattern{.opcode = RV32I_LOAD, .funct3 = 0b001};
const auto LW = I::pattern{.opcode = RV32I_LOAD, .funct3 = 0b010};
const auto LBU = I::pattern{.opcode = RV32I_LOAD, .funct3 = 0b100};
const auto LHU = I::pattern{.opcode = RV32I_LOAD, .funct3 = 0b101};
// Stores
const auto SB = S::pattern{.opcode = RV32I_STORE, .funct3 = 0b000};
const auto SH = S::pattern{.opcode = RV32I_STORE, .funct3 = 0b001};
const auto SW = S::pattern{.opcode = RV32I_STORE, .funct3 = 0b010};
// Math
const auto ADDI = I::pattern{.opcode = RV32I_OP_IMM, .funct3 = 0b000};
// NOP is ADDI x0,x0,0
const auto SLTI = I::pattern{.opcode = RV32I_OP_IMM, .funct3 = 0b010};
const auto SLTIU = I::pattern{.opcode = RV32I_OP_IMM, .funct3 = 0b011};
const auto XORI = I::pattern{.opcode = RV32I_OP_IMM, .funct3 = 0b100};
const auto ORI = I::pattern{.opcode = RV32I_OP_IMM, .funct3 = 0b110};
const auto ANDI = I::pattern{.opcode = RV32I_OP_IMM, .funct3 = 0b111};
const auto SLLI = ConstantShiftFormat::pattern{.opcode = RV32I_OP_IMM, .funct3 = 0b001, .shift_type = 0};
const auto SRLI = ConstantShiftFormat::pattern{.opcode = RV32I_OP_IMM, .funct3 = 0b101, .shift_type = 0};
const auto SRAI = ConstantShiftFormat::pattern{.opcode = RV32I_OP_IMM, .funct3 = 0b101, .shift_type = 1};

const auto ADD = R::pattern{.opcode = RV32I_OP, .funct7 = 0b000'0000, .funct3 = 0b000};
const auto SUB = R::pattern{.opcode = RV32I_OP, .funct7 = 0b010'0000, .funct3 = 0b000};
const auto SLL = R::pattern{.opcode = RV32I_OP, .funct7 = 0b000'0000, .funct3 = 0b001};
const auto SLT = R::pattern{.opcode = RV32I_OP, .funct7 = 0b000'0000, .funct3 = 0b010};
const auto SLTU = R::pattern{.opcode = RV32I_OP, .funct7 = 0b000'0000, .funct3 = 0b011};
const auto XOR = R::pattern{.opcode = RV32I_OP, .funct7 = 0b000'0000, .funct3 = 0b100};
const auto SRL = R::pattern{.opcode = RV32I_OP, .funct7 = 0b000'0000, .funct3 = 0b101};
const auto SRA = R::pattern{.opcode = RV32I_OP, .funct7 = 0b010'0000, .funct3 = 0b101};
const auto OR = R::pattern{.opcode = RV32I_OP, .funct7 = 0b000'0000, .funct3 = 0b110};
const auto AND = R::pattern{.opcode = RV32I_OP, .funct7 = 0b000'0000, .funct3 = 0b111};

// Technically needs to set RS1=0, RD=0
// At the assembly level, takes magic identifiers as argument.
const auto FENCE = I::pattern{.opcode = RV32I_FENCE, .funct3 = 0b000};
const auto FENCE_TSO = FenceFormat::pattern{.fm = 1, .pred = 0b0011, .succ = 0b0011};
const auto PAUSE = FenceFormat::pattern{.fm = 0, .pred = 0b0001, .succ = 0b0000};
const auto ECALL = SystemFormat::pattern{.funct12 = 0};
const auto EBREAK = SystemFormat::pattern{.funct12 = 1};

} // namespace riscv
