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
#include "core/arch/riscv/asmb/rv_mnemonics.hpp"
#include "core/arch/riscv/isa/rv_instruction_list.hpp"
#include "core/arch/riscv/isa/rvi.hpp"

namespace riscv {
static const auto LUI = MnemonicU(RV32I_LUI);
static const auto AUIPC = MnemonicU(RV32I_AUIPC);
// Branches
// Assume rs=1 if not provided
static const auto JAL = MnemonicJ(RV32I_JAL);
// jump: j offset -> jal x0, offset
static const auto J = MnemonicJ(JAL).with_rd(0);
// Assume rs=1 if not provided
static const auto JALR = MnemonicI{RV32I_JALR, 0b000};
// jump register: jr rs -> jalr x0, rs, 0
static const auto JR = MnemonicI(JALR).with_rd(0).with_immediate(0);
static const auto RET = MnemonicI(JALR).with_rd(0).with_rs1(1).with_immediate(0);
static const auto BEQ = MnemonicB(RV32I_BRANCH, 0b000);
static const auto BNE = MnemonicB{RV32I_BRANCH, 0b001};
static const auto BLT = MnemonicB{RV32I_BRANCH, 0b100};
static const auto BGE = MnemonicB{RV32I_BRANCH, 0b101};
static const auto BLTU = MnemonicB{RV32I_BRANCH, 0b110};
static const auto BGEU = MnemonicB{RV32I_BRANCH, 0b111};
// Loads
static const auto LB = MnemonicI{RV32I_LOAD, 0b000};
static const auto LH = MnemonicI{RV32I_LOAD, 0b001};
static const auto LW = MnemonicI{RV32I_LOAD, 0b010};
static const auto LBU = MnemonicI{RV32I_LOAD, 0b100};
static const auto LHU = MnemonicI{RV32I_LOAD, 0b101};
// Stores
static const auto SB = MnemonicS{RV32I_STORE, 0b000};
static const auto SH = MnemonicS{RV32I_STORE, 0b001};
static const auto SW = MnemonicS{RV32I_STORE, 0b010};
// Math
static const auto ADDI = MnemonicI{RV32I_OP_IMM, 0b000};
static const auto NOP = MnemonicI{RV32I_OP_IMM, 0b000}.with_rd(0).with_rs1(0).with_immediate(0);
// NOP is ADDI x0,x0,0
static const auto SLTI = MnemonicI{RV32I_OP_IMM, 0b010};
static const auto SLTIU = MnemonicI{RV32I_OP_IMM, 0b011};
static const auto XORI = MnemonicI{RV32I_OP_IMM, 0b100};
static const auto ORI = MnemonicI{RV32I_OP_IMM, 0b110};
static const auto ANDI = MnemonicI{RV32I_OP_IMM, 0b111};
static const auto SLLI = ConstantShiftMnemonic(RV32I_OP_IMM, 0b001).with_shift_type(0);
static const auto SRLI = ConstantShiftMnemonic(RV32I_OP_IMM, 0b101).with_shift_type(0);
static const auto SRAI = ConstantShiftMnemonic(RV32I_OP_IMM, 0b101).with_shift_type(1);

static const auto ADD = MnemonicR(RV32I_OP, 0b000, 0b000'0000);
static const auto SUB = MnemonicR(RV32I_OP, 0b000, 0b010'0000);
static const auto SLL = MnemonicR(RV32I_OP, 0b001, 0b000'0000);
static const auto SLT = MnemonicR(RV32I_OP, 0b010, 0b000'0000);
static const auto SLTU = MnemonicR(RV32I_OP, 0b011, 0b000'0000);
static const auto XOR = MnemonicR(RV32I_OP, 0b100, 0b000'0000);
static const auto SRL = MnemonicR(RV32I_OP, 0b101, 0b000'0000);
static const auto SRA = MnemonicR(RV32I_OP, 0b101, 0b010'0000);
static const auto OR = MnemonicR(RV32I_OP, 0b110, 0b000'0000);
static const auto AND = MnemonicR(RV32I_OP, 0b111, 0b000'0000);

// At the assembly level, takes magic identifiers as argument.
static const auto FENCE = MnemonicI{RV32I_FENCE, 0b000};
static const auto FENCE_TSO = FenceFormat().with_fm(1).with_pred(0b0011).with_succ(0b0011);
static const auto PAUSE = FenceFormat().with_fm(0).with_pred(0b0001).with_succ(0b0000);
static const auto ECALL = MnemonicI{RV32I_SYSTEM, 0b000}.with_immediate(0);
static const auto EBREAK = MnemonicI{RV32I_SYSTEM, 0b000}.with_immediate(1);

} // namespace riscv
