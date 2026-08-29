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

#include "core/sim/cores/cpu/rv32/rv_i_instructions.hpp"
#include <stdexcept>
#include <string>

namespace {
[[noreturn]] void todo(const char *name) {
  throw std::logic_error(std::string(name) + " is not implemented");
}
} // namespace


// Upper immediate
void handle_lui(RV32CPU *, riscv::InstructionU) { todo("handle_lui"); }
void handle_auipc(RV32CPU *, riscv::InstructionU) { todo("handle_auipc"); }

// Unconditional jumps
void handle_jal(RV32CPU *, riscv::InstructionJ) { todo("handle_jal"); }
void handle_jalr(RV32CPU *, riscv::InstructionI) { todo("handle_jalr"); }

// Conditional branches
void handle_beq(RV32CPU *, riscv::InstructionB) { todo("handle_beq"); }
void handle_bne(RV32CPU *, riscv::InstructionB) { todo("handle_bne"); }
void handle_blt(RV32CPU *, riscv::InstructionB) { todo("handle_blt"); }
void handle_bge(RV32CPU *, riscv::InstructionB) { todo("handle_bge"); }
void handle_bltu(RV32CPU *, riscv::InstructionB) { todo("handle_bltu"); }
void handle_bgeu(RV32CPU *, riscv::InstructionB) { todo("handle_bgeu"); }

// Loads
void handle_lb(RV32CPU *, riscv::InstructionI) { todo("handle_lb"); }
void handle_lh(RV32CPU *, riscv::InstructionI) { todo("handle_lh"); }
void handle_lw(RV32CPU *, riscv::InstructionI) { todo("handle_lw"); }
void handle_lbu(RV32CPU *, riscv::InstructionI) { todo("handle_lbu"); }
void handle_lhu(RV32CPU *, riscv::InstructionI) { todo("handle_lhu"); }

// Stores
void handle_sb(RV32CPU *, riscv::InstructionS) { todo("handle_sb"); }
void handle_sh(RV32CPU *, riscv::InstructionS) { todo("handle_sh"); }
void handle_sw(RV32CPU *, riscv::InstructionS) { todo("handle_sw"); }

// Register-immediate ALU
void handle_addi(RV32CPU *, riscv::InstructionI) { todo("handle_addi"); }
void handle_slti(RV32CPU *, riscv::InstructionI) { todo("handle_slti"); }
void handle_sltiu(RV32CPU *, riscv::InstructionI) { todo("handle_sltiu"); }
void handle_xori(RV32CPU *, riscv::InstructionI) { todo("handle_xori"); }
void handle_ori(RV32CPU *, riscv::InstructionI) { todo("handle_ori"); }
void handle_andi(RV32CPU *, riscv::InstructionI) { todo("handle_andi"); }

// Register-immediate shifts
void handle_slli(RV32CPU *, riscv::InstructionI) { todo("handle_slli"); }
void handle_srli(RV32CPU *, riscv::InstructionI) { todo("handle_srli"); }
void handle_srai(RV32CPU *, riscv::InstructionI) { todo("handle_srai"); }

// Register-register ALU
void handle_add(RV32CPU *, riscv::InstructionR) { todo("handle_add"); }
void handle_sub(RV32CPU *, riscv::InstructionR) { todo("handle_sub"); }
void handle_sll(RV32CPU *, riscv::InstructionR) { todo("handle_sll"); }
void handle_slt(RV32CPU *, riscv::InstructionR) { todo("handle_slt"); }
void handle_sltu(RV32CPU *, riscv::InstructionR) { todo("handle_sltu"); }
void handle_xor(RV32CPU *, riscv::InstructionR) { todo("handle_xor"); }
void handle_srl(RV32CPU *, riscv::InstructionR) { todo("handle_srl"); }
void handle_sra(RV32CPU *, riscv::InstructionR) { todo("handle_sra"); }
void handle_or(RV32CPU *, riscv::InstructionR) { todo("handle_or"); }
void handle_and(RV32CPU *, riscv::InstructionR) { todo("handle_and"); }

// Memory ordering
void handle_fence(RV32CPU *, riscv::InstructionI) { todo("handle_fence"); }
void handle_fence_tso(RV32CPU *, riscv::InstructionI) { todo("handle_fence_tso"); }

// Environment. I-type encodings, though neither reads an operand.
void handle_ecall(RV32CPU *, riscv::InstructionI) { todo("handle_ecall"); }
void handle_ebreak(RV32CPU *, riscv::InstructionI) { todo("handle_ebreak"); }
