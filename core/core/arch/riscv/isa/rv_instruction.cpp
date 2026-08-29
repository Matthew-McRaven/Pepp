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
 */
#include "core/arch/riscv/isa/rv_instruction.hpp"
#include "core/arch/riscv/isa/rvi.hpp"
#include "core/arch/riscv/isa/rv_instruction_format.hpp"

namespace riscv {
namespace {
// imm[11:5] on RV32. SLLI/SRLI require 0000000 and SRAI requires 0100000; imm[5] set means
// shamt >= 32, which is reserved on RV32 even though RV64 uses it as the sixth shamt bit.
constexpr u32 SHIFT_HIGH = 0xFE0;
constexpr u32 SHIFT_LOGICAL = 0x000;
constexpr u32 SHIFT_ARITHMETIC = 0x400;
// FENCE.TSO is fm=1000 with pred=succ=rw, i.e. the whole 12-bit immediate is 0x833.
constexpr u32 FENCE_TSO_IMM = 0x833;
} // namespace

RvOp decode(rv_instruction2 w) noexcept {
  // Compressed encodings are the C extension; nothing in RV32I lives below quadrant 3.
  if (!w.is_long()) return RvOp::INVALID;

  switch (w.opcode()) {
  case RV32I_LOAD:
    switch (w.as<InstructionI>().funct3) {
    case 0b000: return RvOp::LB;
    case 0b001: return RvOp::LH;
    case 0b010: return RvOp::LW;
    case 0b100: return RvOp::LBU;
    case 0b101: return RvOp::LHU;
    // 011 (LD) and 110 (LWU) are RV64-only; 111 is reserved.
    default: return RvOp::INVALID;
    }

  case RV32I_STORE:
    switch (w.as<InstructionS>().funct3) {
    case 0b000: return RvOp::SB;
    case 0b001: return RvOp::SH;
    case 0b010: return RvOp::SW;
    // 011 (SD) is RV64-only.
    default: return RvOp::INVALID;
    }

  case RV32I_BRANCH:
    switch (w.as<InstructionB>().funct3) {
    case 0b000: return RvOp::BEQ;
    case 0b001: return RvOp::BNE;
    case 0b100: return RvOp::BLT;
    case 0b101: return RvOp::BGE;
    case 0b110: return RvOp::BLTU;
    case 0b111: return RvOp::BGEU;
    // 010 and 011 are reserved.
    default: return RvOp::INVALID;
    }

  case RV32I_JALR: return w.as<InstructionI>().funct3 == 0b000 ? RvOp::JALR : RvOp::INVALID;
  case RV32I_JAL: return RvOp::JAL;
  case RV32I_LUI: return RvOp::LUI;
  case RV32I_AUIPC: return RvOp::AUIPC;

  case RV32I_OP_IMM: {
    const auto i = w.as<InstructionI>();
    switch (i.funct3) {
    case 0b000: return RvOp::ADDI;
    case 0b010: return RvOp::SLTI;
    case 0b011: return RvOp::SLTIU;
    case 0b100: return RvOp::XORI;
    case 0b110: return RvOp::ORI;
    case 0b111: return RvOp::ANDI;
    case 0b001: return (i.imm & SHIFT_HIGH) == SHIFT_LOGICAL ? RvOp::SLLI : RvOp::INVALID;
    case 0b101:
      switch (i.imm & SHIFT_HIGH) {
      case SHIFT_LOGICAL: return RvOp::SRLI;
      case SHIFT_ARITHMETIC: return RvOp::SRAI;
      default: return RvOp::INVALID;
      }
    default: return RvOp::INVALID;
    }
  }

  case RV32I_OP: {
    const auto r = w.as<InstructionR>();
    switch (r.funct7) {
    case 0b0000000: // base register-register
      switch (r.funct3) {
      case 0b000: return RvOp::ADD;
      case 0b001: return RvOp::SLL;
      case 0b010: return RvOp::SLT;
      case 0b011: return RvOp::SLTU;
      case 0b100: return RvOp::XOR;
      case 0b101: return RvOp::SRL;
      case 0b110: return RvOp::OR;
      case 0b111: return RvOp::AND;
      default: return RvOp::INVALID;
      }
    case 0b0100000: // subtract and arithmetic shift
      switch (r.funct3) {
      case 0b000: return RvOp::SUB;
      case 0b101: return RvOp::SRA;
      default: return RvOp::INVALID;
      }
    // 0000001 is M. The bit-manipulation extensions claim further rows
    default: return RvOp::INVALID;
    }
  }

  case RV32I_FENCE: {
    const auto i = w.as<InstructionI>();
    // funct3 001 is FENCE.I, which belongs to Zifencei rather than the base set.
    if (i.funct3 != 0b000) return RvOp::INVALID;
    return i.imm == FENCE_TSO_IMM ? RvOp::FENCE_TSO : RvOp::FENCE;
  }

  case RV32I_SYSTEM: {
    const auto i = w.as<InstructionI>();
    // The CSR instructions occupy the other funct3 values and belong to Zicsr.
    if (i.funct3 != 0b000) return RvOp::INVALID;
    switch (i.imm) {
    case 0: return RvOp::ECALL;
    case 1: return RvOp::EBREAK;
    // MRET/SRET/WFI are privileged, not part of RV32I.
    default: return RvOp::INVALID;
    }
  }

  default: return RvOp::INVALID;
  }
}

std::string rv_instruction2::to_string() const {
  const auto &info = op_info(decode(*this));
  switch (info.syntax) {
  case RvSyntax::R: return fmt_r_type(info.name, as<InstructionR>());
  case RvSyntax::I_ALU: return fmt_i_alu(info.name, as<InstructionI>());
  case RvSyntax::I_Shift: return fmt_i_shift(info.name, as<InstructionI>());
  case RvSyntax::I_Offset: return fmt_i_offset(info.name, as<InstructionI>());
  case RvSyntax::S: return fmt_s_store(info.name, as<InstructionS>());
  case RvSyntax::B: return fmt_b_branch(info.name, as<InstructionB>());
  case RvSyntax::U: return fmt_u_upper(info.name, as<InstructionU>());
  case RvSyntax::J: return fmt_j_jal(info.name, as<InstructionJ>());
  case RvSyntax::I_Fence: return fmt_fence(info.name, as<InstructionI>());
  case RvSyntax::I_NoOperands: return fmt_no_operands(info.name);
  case RvSyntax::Unknown: break;
  }
  return fmt_unknown(bits(), is_compressed());
}
} // namespace riscv
