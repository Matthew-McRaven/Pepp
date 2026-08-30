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
#include "core/arch/riscv/isa/rv_instruction_format.hpp"
#include <array>
#include "core/arch/riscv/isa/rv_base.hpp"
#include "fmt/format.h"
#include "fmt/ranges.h"

namespace {
// Unify formatting of parts / columns as in pep's formatters
template <typename... Args> std::string format_instruction(std::string_view mnemonic, Args &&...operands) {
  if constexpr (sizeof...(operands) == 0) return std::string(mnemonic);
  else {
    const std::array<std::string, sizeof...(operands)> parts{fmt::format("{}", operands)...};
    return fmt::format("{} {}", mnemonic, fmt::join(parts, ", "));
  }
}

// One nibble of a FENCE's pred orsucc field, with an empty set printing as 0.
std::string iorw(uint32_t nibble) {
  std::string out;
  if (nibble & 0b1000) out += 'i';
  if (nibble & 0b0100) out += 'o';
  if (nibble & 0b0010) out += 'r';
  if (nibble & 0b0001) out += 'w';
  return out.empty() ? std::string("0") : out;
}
} // namespace

std::string riscv::fmt_r_type(std::string_view mnemonic, InstructionR i) {
  return format_instruction(mnemonic, xname(i.rd), xname(i.rs1), xname(i.rs2));
}

std::string riscv::fmt_i_alu(std::string_view mnemonic, InstructionI i) {
  return format_instruction(mnemonic, xname(i.rd), xname(i.rs1), i.signed_imm());
}

std::string riscv::fmt_i_shift(std::string_view mnemonic, InstructionI i) {
  return format_instruction(mnemonic, xname(i.rd), xname(i.rs1), i.shift_imm());
}

std::string riscv::fmt_i_offset(std::string_view mnemonic, InstructionI i) {
  return format_instruction(mnemonic, xname(i.rd), fmt::format("{}({})", i.signed_imm(), xname(i.rs1)));
}

std::string riscv::fmt_s_store(std::string_view mnemonic, InstructionS i) {
  return format_instruction(mnemonic, xname(i.rs2), fmt::format("{}({})", i.signed_imm(), xname(i.rs1)));
}

std::string riscv::fmt_b_branch(std::string_view mnemonic, InstructionB i) {
  return format_instruction(mnemonic, xname(i.rs1), xname(i.rs2), i.signed_imm());
}

std::string riscv::fmt_u_upper(std::string_view mnemonic, InstructionU i) {
  // Must copy out of bit-field because fmt cannot bind to it.
  const uint32_t upper = i.imm;
  return format_instruction(mnemonic, xname(i.rd), fmt::format("{:#x}", upper));
}

std::string riscv::fmt_j_jal(std::string_view mnemonic, InstructionJ i) {
  return format_instruction(mnemonic, xname(i.rd), i.jump_offset());
}

std::string riscv::fmt_fence(std::string_view mnemonic, InstructionI i) {
  // imm[7:4] is pred and imm[3:0] is succ; imm[11:8] is fm, which FENCE.TSO uses and a
  // plain FENCE leaves zero.
  return format_instruction(mnemonic, iorw((i.imm >> 4) & 0xF), iorw(i.imm & 0xF));
}

std::string riscv::fmt_no_operands(std::string_view mnemonic) { return format_instruction(mnemonic); }

std::string riscv::fmt_unknown(uint32_t bits, bool compressed) {
  return compressed ? fmt::format("unknown {:#06x}", bits & 0xFFFF) : fmt::format("unknown {:#010x}", bits);
}