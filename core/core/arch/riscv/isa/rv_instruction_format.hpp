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
#include <string>
#include <string_view>
#include "core/arch/riscv/isa/rvi.hpp"

// Formatting helpers for ISA-level disassembly. Uses architectural names for registers,
// and does not resolve psuedo-instructions.
namespace riscv {

// rd, rs1, rs2
std::string fmt_r_type(std::string_view mnemonic, InstructionR i);
// rd, rs1, 12-bit sign-extended imm
std::string fmt_i_alu(std::string_view mnemonic, InstructionI i);
// rd, rs1, 5-bit shamt
std::string fmt_i_shift(std::string_view mnemonic, InstructionI i);
// rd, imm(rs1)
std::string fmt_i_offset(std::string_view mnemonic, InstructionI i);
// rs2, imm(rs1)
std::string fmt_s_store(std::string_view mnemonic, InstructionS i);
// rs1, rs2, offset
std::string fmt_b_branch(std::string_view mnemonic, InstructionB i);
// rd, 0xNNNNN
std::string fmt_u_upper(std::string_view mnemonic, InstructionU i);
// rd, offset
std::string fmt_j_jal(std::string_view mnemonic, InstructionJ i);
// pred, succ as iorw letters
std::string fmt_fence(std::string_view mnemonic, InstructionI i);
// Only operand, no arguments. FENCE.TSO, ECALL, EBREAK.
std::string fmt_no_operands(std::string_view mnemonic);
// Anything the decoder rejected, rendered as its raw encoding.
std::string fmt_unknown(uint32_t bits, bool compressed);
} // namespace riscv
