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
static const auto _RD = Operand{.type = Operand::Type::Register, .destination = Operand::Destination::RD};
static const auto _RS1 = Operand{.type = Operand::Type::Register, .destination = Operand::Destination::RS1};
static const auto _RS2 = Operand{.type = Operand::Type::Register, .destination = Operand::Destination::RS2};
static const auto _RS = Operand{.type = Operand::Type::Register, .destination = Operand::Destination::RS};
static const auto _IMM = Operand{.type = Operand::Type::Immediate, .destination = Operand::Destination::IMM};
static const auto _SHAMT = Operand{.type = Operand::Type::Immediate, .destination = Operand::Destination::SHAMT};
static const auto _PRED = Operand{.type = Operand::Type::Fence, .destination = Operand::Destination::PRED};
static const auto _SUCC = Operand{.type = Operand::Type::Fence, .destination = Operand::Destination::SUCC};
static const auto _XLEN8 = Operand{.type = Operand::Type::XLEN8, .destination = Operand::Destination::IMM};
static const auto _XLEN16 = Operand{.type = Operand::Type::XLEN16, .destination = Operand::Destination::IMM};

// Integer-Register-Immediate instructions
static const auto ADDI = MnemonicDescriptor::I(RV32I_OP_IMM, 0b000).with_operand(_RD, _RS1, _IMM);
static const auto SLTI = MnemonicDescriptor::I(RV32I_OP_IMM, 0b010).with_operand(_RD, _RS1, _IMM);
static const auto SLTIU = MnemonicDescriptor::I(RV32I_OP_IMM, 0b011).with_operand(_RD, _RS1, _IMM);
static const auto ANDI = MnemonicDescriptor::I(RV32I_OP_IMM, 0b111).with_operand(_RD, _RS1, _IMM);
static const auto XORI = MnemonicDescriptor::I(RV32I_OP_IMM, 0b100).with_operand(_RD, _RS1, _IMM);
static const auto ORI = MnemonicDescriptor::I(RV32I_OP_IMM, 0b110).with_operand(_RD, _RS1, _IMM);
static const auto SLLI = MnemonicDescriptor::IShiftByConstant(RV32I_OP_IMM, 0b001, 0).with_operand(_RD, _RS1, _SHAMT);
static const auto SRLI = MnemonicDescriptor::IShiftByConstant(RV32I_OP_IMM, 0b101, 0).with_operand(_RD, _RS1, _SHAMT);
static const auto SRAI = MnemonicDescriptor::IShiftByConstant(RV32I_OP_IMM, 0b101, 1).with_operand(_RD, _RS1, _SHAMT);
static const auto LUI = MnemonicDescriptor::U(RV32I_LUI).with_operand(_RD, _IMM);
static const auto AUIPC = MnemonicDescriptor::U(RV32I_AUIPC).with_operand(_RD, _IMM);

// Integer-Register-Register instructions
static const auto ADD = MnemonicDescriptor::R(RV32I_OP, 0b000, 0b000'0000).with_operand(_RD, _RS1, _RS2);
static const auto SLT = MnemonicDescriptor::R(RV32I_OP, 0b010, 0b000'0000).with_operand(_RD, _RS1, _RS2);
static const auto SLTU = MnemonicDescriptor::R(RV32I_OP, 0b011, 0b000'0000).with_operand(_RD, _RS1, _RS2);
// and / or /xor
static const auto AND = MnemonicDescriptor::R(RV32I_OP, 0b111, 0b000'0000).with_operand(_RD, _RS1, _RS2);
static const auto OR = MnemonicDescriptor::R(RV32I_OP, 0b110, 0b000'0000).with_operand(_RD, _RS1, _RS2);
static const auto XOR = MnemonicDescriptor::R(RV32I_OP, 0b100, 0b000'0000).with_operand(_RD, _RS1, _RS2);
// sll and srl
static const auto SLL = MnemonicDescriptor::R(RV32I_OP, 0b001, 0b000'0000).with_operand(_RD, _RS1, _RS2);
static const auto SRL = MnemonicDescriptor::R(RV32I_OP, 0b101, 0b000'0000).with_operand(_RD, _RS1, _RS2);
// sub andr sra
static const auto SUB = MnemonicDescriptor::R(RV32I_OP, 0b000, 0b010'0000).with_operand(_RD, _RS1, _RS2);
static const auto SRA = MnemonicDescriptor::R(RV32I_OP, 0b101, 0b010'0000).with_operand(_RD, _RS1, _RS2);

// Unconditional Control Transfer instructions
// Allows either JAL rd, offset or JAL offset. If second variant, use default rd.
static const auto JAL = MnemonicDescriptor::J(RV32I_JAL).with_operand(_RD, _IMM).with_rd(0);
static const auto JALR = MnemonicDescriptor::I(RV32I_JALR, 0b000).with_operand(_RD, _RS1, _IMM).with_rs1(1);

// Conditional Control Transfer instructions
static const auto BEQ = MnemonicDescriptor::B(RV32I_BRANCH, 0b000).with_operand(_RS1, _RS2, _IMM);
static const auto BNE = MnemonicDescriptor::B(RV32I_BRANCH, 0b001).with_operand(_RS1, _RS2, _IMM);
static const auto BLT = MnemonicDescriptor::B(RV32I_BRANCH, 0b100).with_operand(_RS1, _RS2, _IMM);
static const auto BLTU = MnemonicDescriptor::B(RV32I_BRANCH, 0b110).with_operand(_RS1, _RS2, _IMM);
static const auto BGE = MnemonicDescriptor::B(RV32I_BRANCH, 0b101).with_operand(_RS1, _RS2, _IMM);
static const auto BGEU = MnemonicDescriptor::B(RV32I_BRANCH, 0b111).with_operand(_RS1, _RS2, _IMM);

// Load and Store instructions
static const auto LB = MnemonicDescriptor::I(RV32I_LOAD, 0b000).with_operand(_RD, _RS1, _IMM);
static const auto LH = MnemonicDescriptor::I(RV32I_LOAD, 0b001).with_operand(_RD, _RS1, _IMM);
static const auto LW = MnemonicDescriptor::I(RV32I_LOAD, 0b010).with_operand(_RD, _RS1, _IMM);
static const auto LBU = MnemonicDescriptor::I(RV32I_LOAD, 0b100).with_operand(_RD, _RS1, _IMM);
static const auto LHU = MnemonicDescriptor::I(RV32I_LOAD, 0b101).with_operand(_RD, _RS1, _IMM);
static const auto SB = MnemonicDescriptor::S(RV32I_STORE, 0b000).with_operand(_RS1, _RS2, _IMM);
static const auto SH = MnemonicDescriptor::S(RV32I_STORE, 0b001).with_operand(_RS1, _RS2, _IMM);
static const auto SW = MnemonicDescriptor::S(RV32I_STORE, 0b010).with_operand(_RS1, _RS2, _IMM);

// Memory Ordering instructions
static const auto FENCE = MnemonicDescriptor::IFence(0b0000).with_operand(_PRED, _SUCC).with_rs1(0).with_rd(0);
static const auto FENCE_TSO = MnemonicDescriptor::IFence(0b1000, 0b0011, 0b0011).with_rs1(0).with_rd(0);
static const auto ECALL = MnemonicDescriptor::I(RV32I_SYSTEM, 0b000).with_rs1(0).with_rd(0).with_imm(0);
static const auto EBREAK = MnemonicDescriptor::I(RV32I_SYSTEM, 0b000).with_rs1(0).with_rd(0).with_imm(1);

// Pseudo-instructions
static const auto J = MnemonicDescriptor::J(RV32I_JAL).with_operand(_IMM).with_rd(0); // PI
static const auto JR = MnemonicDescriptor::I(RV32I_JALR, 0b000).with_operand(_RS).with_rd(0).with_imm(0);
static const auto RET = MnemonicDescriptor::I(RV32I_JALR, 0b000).with_rd(0).with_imm(0).with_rs1(1);
static const auto PAUSE = MnemonicDescriptor::IFence(0b0000, 0b0001, 0b0000).with_rs1(0).with_rd(0);
static const auto NOP = MnemonicDescriptor::I(RV32I_OP_IMM, 0b000).with_rd(0).with_rs1(0).with_imm(0);
// move: mv rd, rs -> addi rd, rs, 0
static const auto MOVE = MnemonicDescriptor::I(RV32I_OP_IMM, 0b000).with_operand(_RD, _RS).with_imm(0);
// not: not rd, rs -> xori rd, rs, -1
static const auto NOT = MnemonicDescriptor::I(RV32I_OP_IMM, 0b100).with_operand(_RD, _RS).with_imm(0xFFFF'FFFF);
// negate: neg rd, rs -> sub rd, x0, rs
static const auto NEGATE = MnemonicDescriptor::R(RV32I_OP, 0b000, 0b010'0000).with_operand(_RD, _RS).with_rs2(0);
// sign extend byte: sext.b rd, rs  -> slli rd, rs, XLEN - 8; srai rd, rd, XLEN - 8
static const auto SEXT_B = MnemonicDescriptor::Pseudo().with_operand(_RD, _RS);
static const auto SEXT_B_pattern = std::array<MnemonicDescriptor, 2>{SLLI.replaced_operands(_RD, _RS, _XLEN8),
                                                                     SRAI.replaced_operands(_RD, _RD, _XLEN8)};
// sign extend halfword: sext.h rd, rs -> slli rd, rs, XLEN - 16; srai rd, rd, XLEN - 16
static const auto SEXT_H = MnemonicDescriptor::Pseudo().with_operand(_RD, _RS);
static const auto SEXT_H_pattern = std::array<MnemonicDescriptor, 2>{SLLI.replaced_operands(_RD, _RS, _XLEN16),
                                                                     SRAI.replaced_operands(_RD, _RD, _XLEN16)};
// zero extend byte: zext.b rd, rs -> andi rd, rs, 255
static const auto ZEXT_B = MnemonicDescriptor(ANDI).replaced_operands(_RD, _RS1).with_imm(0xff);
// zero extend halfword: zext.h rd, rs -> slli rd, rs, XLEN - 16; srli rd, rd, XLEN - 16
static const auto ZEXT_H = MnemonicDescriptor::Pseudo();
static const auto ZEXT_H_pattern = std::array<MnemonicDescriptor, 2>{SLLI.replaced_operands(_RD, _RS, _XLEN16),
                                                                     SRLI.replaced_operands(_RD, _RD, _XLEN16)};
// set if equal to 0: seqz rd, rs -> sltiu rd, rs, 1
static const auto SEQZ = MnemonicDescriptor::I(RV32I_OP_IMM, 0b011).with_operand(_RD, _RS1).with_imm(1);
// set if not equal to 0: snez rd, rs -> sltu rd, x0, rs
static const auto SNEZ = MnemonicDescriptor::R(RV32I_OP, 0b011, 0b000'0000).with_operand(_RD, _RS2).with_rs1(0);
// set if < 0: sltz rd, rs -> slt rd, rs, x0
static const auto SLTZ = MnemonicDescriptor::R(RV32I_OP, 0b010, 0b000'0000).with_operand(_RD, _RS1).with_rs2(0);
// set if > 0: sgtz rd, rs -> slt rd, x0, rs
static const auto SGTZ = MnemonicDescriptor::R(RV32I_OP, 0b010, 0b000'0000).with_operand(_RD, _RS2).with_rs1(0);
// branch if equal to 0: beqz rs, offset -> beq rs, x0, offset
static const auto BEQZ = MnemonicDescriptor::B(RV32I_BRANCH, 0b000).with_operand(_RS1, _IMM).with_rs2(0);
// branch if not equal to 0: bnez rs, offset -> bne rs, x0, offset
static const auto BNEZ = MnemonicDescriptor::B(RV32I_BRANCH, 0b001).with_operand(_RS1, _IMM).with_rs2(0);
// branch if <= 0: blez rs, offset -> bge x0, rs, offset
static const auto BLEZ = MnemonicDescriptor::B(RV32I_BRANCH, 0b101).with_operand(_RS2, _IMM).with_rs1(0);
// branch if >= 0: bgez rs, offset -> bge rs, x0, offset
static const auto BGEZ = MnemonicDescriptor::B(RV32I_BRANCH, 0b101).with_operand(_RS1, _IMM).with_rs2(0);
// branch if < 0: bltz rs, offset -> blt rs, x0, offset
static const auto BLTZ = MnemonicDescriptor::B(RV32I_BRANCH, 0b100).with_operand(_RS1, _IMM).with_rs2(0);
// branch if > 0: bgtz rs, offset -> blt x0, rs, offset
static const auto BGTZ = MnemonicDescriptor::B(RV32I_BRANCH, 0b100).with_operand(_RS2, _IMM).with_rs1(0);
// branch if >: bgt rs1, rs2, offset -> blt rs2, rs1, offset
static const auto BGT = MnemonicDescriptor::B(RV32I_BRANCH, 0b100).with_operand(_RS2, _RS1, _IMM);
// branch if <=: ble rs1, rs2, offset -> bge rs2, rs1, offset
static const auto BLE = MnemonicDescriptor::B(RV32I_BRANCH, 0b101).with_operand(_RS2, _RS1, _IMM);
// branch if > unsigned: bgtu rs1, rs2, offset -> bltu rs2, rs1, offset
static const auto BGTU = MnemonicDescriptor::B(RV32I_BRANCH, 0b110).with_operand(_RS2, _RS1, _IMM);
// branch if <= unsigned: bleu rs1, rs2, offset -> bgeu rs2, rs1, offset
static const auto BLEU = MnemonicDescriptor::B(RV32I_BRANCH, 0b111).with_operand(_RS2, _RS1, _IMM);

} // namespace riscv
