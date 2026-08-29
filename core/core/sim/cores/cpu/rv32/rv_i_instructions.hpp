/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
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
 */

#pragma once
#include "core/arch/riscv/isa/rvi.hpp"

class RV32CPU;

// Upper immediate
void handle_lui(RV32CPU *self, riscv::InstructionU i);
void handle_auipc(RV32CPU *self, riscv::InstructionU i);

// Unconditional jumps
void handle_jal(RV32CPU *self, riscv::InstructionJ i);
void handle_jalr(RV32CPU *self, riscv::InstructionI i);

// Conditional branches
void handle_beq(RV32CPU *self, riscv::InstructionB i);
void handle_bne(RV32CPU *self, riscv::InstructionB i);
void handle_blt(RV32CPU *self, riscv::InstructionB i);
void handle_bge(RV32CPU *self, riscv::InstructionB i);
void handle_bltu(RV32CPU *self, riscv::InstructionB i);
void handle_bgeu(RV32CPU *self, riscv::InstructionB i);

// Loads
void handle_lb(RV32CPU *self, riscv::InstructionI i);
void handle_lh(RV32CPU *self, riscv::InstructionI i);
void handle_lw(RV32CPU *self, riscv::InstructionI i);
void handle_lbu(RV32CPU *self, riscv::InstructionI i);
void handle_lhu(RV32CPU *self, riscv::InstructionI i);

// Stores
void handle_sb(RV32CPU *self, riscv::InstructionS i);
void handle_sh(RV32CPU *self, riscv::InstructionS i);
void handle_sw(RV32CPU *self, riscv::InstructionS i);

// Register-immediate ALU
void handle_addi(RV32CPU *self, riscv::InstructionI i);
void handle_slti(RV32CPU *self, riscv::InstructionI i);
void handle_sltiu(RV32CPU *self, riscv::InstructionI i);
void handle_xori(RV32CPU *self, riscv::InstructionI i);
void handle_ori(RV32CPU *self, riscv::InstructionI i);
void handle_andi(RV32CPU *self, riscv::InstructionI i);

// Register-immediate shifts
void handle_slli(RV32CPU *self, riscv::InstructionI i);
void handle_srli(RV32CPU *self, riscv::InstructionI i);
void handle_srai(RV32CPU *self, riscv::InstructionI i);

// Register-register ALU
void handle_add(RV32CPU *self, riscv::InstructionR i);
void handle_sub(RV32CPU *self, riscv::InstructionR i);
void handle_sll(RV32CPU *self, riscv::InstructionR i);
void handle_slt(RV32CPU *self, riscv::InstructionR i);
void handle_sltu(RV32CPU *self, riscv::InstructionR i);
void handle_xor(RV32CPU *self, riscv::InstructionR i);
void handle_srl(RV32CPU *self, riscv::InstructionR i);
void handle_sra(RV32CPU *self, riscv::InstructionR i);
void handle_or(RV32CPU *self, riscv::InstructionR i);
void handle_and(RV32CPU *self, riscv::InstructionR i);

// Memory ordering
void handle_fence(RV32CPU *self, riscv::InstructionI i);
void handle_fence_tso(RV32CPU *self, riscv::InstructionI i);

// Environment. I-type encodings, though neither reads an operand.
void handle_ecall(RV32CPU *self, riscv::InstructionI i);
void handle_ebreak(RV32CPU *self, riscv::InstructionI i);
