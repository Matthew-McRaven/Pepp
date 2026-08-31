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
#include <type_traits>
#include "core/sim/cores/cpu/rv32/rv_i_ops.hpp"
#include "core/sim/cores/cpu/rv32/rv_isa.hpp"
using XReg = riscv::XReg;
using namespace rv32;
namespace {
[[noreturn]] void todo(const char *name) { throw std::logic_error(std::string(name) + " is not implemented"); }


// The two families differ only in where the right-hand operand comes from.
template <u32 (*Op)(u32, u32)> void reg_reg(RV32CPU *self, riscv::InstructionR r) {
  self->write_register((XReg)r.rd, Op(self->read_register((XReg)r.rs1), self->read_register((XReg)r.rs2)));
}
template <u32 (*Op)(u32, u32)> void reg_imm(RV32CPU *self, riscv::InstructionI i, u32 rhs) {
  self->write_register((XReg)i.rd, Op(self->read_register((XReg)i.rs1), rhs));
}
} // namespace

// Upper immediate
void handle_lui(RV32CPU *self, riscv::InstructionU u) { self->write_register((XReg)u.rd, u.upper_imm()); }
// Relative to this instruction's own address. Needs PC prior to execution.
void handle_auipc(RV32CPU *self, riscv::InstructionU u) {
  self->write_register((XReg)u.rd, self->read_initial_pc() + u.upper_imm());
}

// Unconditional jumps
void handle_jal(RV32CPU *self, riscv::InstructionJ j) {
  const u32 pc = self->read_initial_pc();
  self->write_register((XReg)j.rd, pc + 4);
  self->write_next_pc(pc + u32(j.jump_offset()));
}
void handle_jalr(RV32CPU *self, riscv::InstructionI i) {
  const u32 pc = self->read_initial_pc();
  // The target is formed before rd is written, because rd and rs1 may name the same register.
  // Unlike a branch this is register-relative rather than PC-relative, and the spec requires
  // the low bit of the result to be cleared rather than treated as a misalignment.
  const u32 target = (self->read_register((XReg)i.rs1) + u32(i.signed_imm())) & ~u32(1);
  self->write_register((XReg)i.rd, pc + 4);
  self->write_next_pc(target);
}

// Conditional branches
namespace {
// The six branches differ only in how they compare, and whether the comparison is signed.
template <typename T, typename Cmp> void branch(RV32CPU *self, riscv::InstructionB b, Cmp cmp) {
  const auto rs1 = static_cast<T>(self->read_register((XReg)b.rs1));
  const auto rs2 = static_cast<T>(self->read_register((XReg)b.rs2));
  if (cmp(rs1, rs2)) self->write_next_pc(self->read_initial_pc() + b.signed_imm());
}
} // namespace
void handle_beq(RV32CPU *self, riscv::InstructionB b) { branch<i32>(self, b, std::equal_to{}); }
void handle_bne(RV32CPU *self, riscv::InstructionB b) { branch<i32>(self, b, std::not_equal_to{}); }
void handle_blt(RV32CPU *self, riscv::InstructionB b) { branch<i32>(self, b, std::less{}); }
void handle_bge(RV32CPU *self, riscv::InstructionB b) { branch<i32>(self, b, std::greater_equal{}); }
void handle_bltu(RV32CPU *self, riscv::InstructionB b) { branch<u32>(self, b, std::less{}); }
void handle_bgeu(RV32CPU *self, riscv::InstructionB b) { branch<u32>(self, b, std::greater_equal{}); }

namespace {
// The width and sign-/zero-extension both derivce from T. Always assume LE for the target.
template <typename T> void load(RV32CPU *self, riscv::InstructionI i) {
  const u32 addr = self->read_register((XReg)i.rs1) + u32(i.signed_imm());
  const auto raw = self->target()->read<std::make_unsigned_t<T>, !bits::host_is_le>(addr, self->op_data()).second;
  self->write_register((XReg)i.rd, u32(static_cast<T>(raw)));
}
template <typename T> void store(RV32CPU *self, riscv::InstructionS s) {
  const u32 addr = self->read_register((XReg)s.rs1) + u32(s.signed_imm());
  const auto value = self->read_register((XReg)s.rs2);
  self->target()->write<T, !bits::host_is_le>(addr, value, self->op_data());
}
} // namespace
void handle_lb(RV32CPU *self, riscv::InstructionI i) { load<i8>(self, i); }
void handle_lh(RV32CPU *self, riscv::InstructionI i) { load<i16>(self, i); }
void handle_lw(RV32CPU *self, riscv::InstructionI i) { load<i32>(self, i); }
void handle_lbu(RV32CPU *self, riscv::InstructionI i) { load<u8>(self, i); }
void handle_lhu(RV32CPU *self, riscv::InstructionI i) { load<u16>(self, i); }

// Stores
void handle_sb(RV32CPU *self, riscv::InstructionS s) { store<i8>(self, s); }
void handle_sh(RV32CPU *self, riscv::InstructionS s) { store<i16>(self, s); }
void handle_sw(RV32CPU *self, riscv::InstructionS s) { store<i32>(self, s); }

// Register-immediate ALU
// Every immediate here is sign-extended before use.
void handle_addi(RV32CPU *self, riscv::InstructionI i) { reg_imm<op_add>(self, i, u32(i.signed_imm())); }
void handle_slti(RV32CPU *self, riscv::InstructionI i) { reg_imm<op_slt>(self, i, u32(i.signed_imm())); }
void handle_sltiu(RV32CPU *self, riscv::InstructionI i) { reg_imm<op_sltu>(self, i, u32(i.signed_imm())); }
void handle_xori(RV32CPU *self, riscv::InstructionI i) { reg_imm<op_xor>(self, i, u32(i.signed_imm())); }
void handle_ori(RV32CPU *self, riscv::InstructionI i) { reg_imm<op_or>(self, i, u32(i.signed_imm())); }
void handle_andi(RV32CPU *self, riscv::InstructionI i) { reg_imm<op_and>(self, i, u32(i.signed_imm())); }

// Register-immediate shifts
// shift_imm() is imm[4:0]
void handle_slli(RV32CPU *self, riscv::InstructionI i) { reg_imm<op_sll>(self, i, i.shift_imm()); }
void handle_srli(RV32CPU *self, riscv::InstructionI i) { reg_imm<op_srl>(self, i, i.shift_imm()); }
void handle_srai(RV32CPU *self, riscv::InstructionI i) { reg_imm<op_sra>(self, i, i.shift_imm()); }

// Register-register ALU
void handle_add(RV32CPU *self, riscv::InstructionR r) { reg_reg<op_add>(self, r); }
void handle_sub(RV32CPU *self, riscv::InstructionR r) { reg_reg<op_sub>(self, r); }
void handle_sll(RV32CPU *self, riscv::InstructionR r) { reg_reg<op_sll>(self, r); }
void handle_slt(RV32CPU *self, riscv::InstructionR r) { reg_reg<op_slt>(self, r); }
void handle_sltu(RV32CPU *self, riscv::InstructionR r) { reg_reg<op_sltu>(self, r); }
void handle_xor(RV32CPU *self, riscv::InstructionR r) { reg_reg<op_xor>(self, r); }
void handle_srl(RV32CPU *self, riscv::InstructionR r) { reg_reg<op_srl>(self, r); }
void handle_sra(RV32CPU *self, riscv::InstructionR r) { reg_reg<op_sra>(self, r); }
void handle_or(RV32CPU *self, riscv::InstructionR r) { reg_reg<op_or>(self, r); }
void handle_and(RV32CPU *self, riscv::InstructionR r) { reg_reg<op_and>(self, r); }

// Memory ordering
void handle_fence(RV32CPU *self, riscv::InstructionI i) {
  // Intentional no-op for now.
}
void handle_fence_tso(RV32CPU *self, riscv::InstructionI i) {
  // Intentional no-op for now.
}

// Environment. I-type encodings, though neither reads an operand.
void handle_ecall(RV32CPU *self, riscv::InstructionI i) { todo("handle_ecall"); }
void handle_ebreak(RV32CPU *self, riscv::InstructionI i) { todo("handle_ebreak"); }
