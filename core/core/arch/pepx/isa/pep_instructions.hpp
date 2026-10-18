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
#include <array>
#include <bitset>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include "core/arch/pepx/isa/pep_opcodes.hpp"
#include "core/arch/pepx/isa/pep_registers.hpp"
#include "core/integers.h"

namespace pep {
// One row per mnemonic with the base mnemonic, converted into a number of opcodes based on the low_bits patterns
struct InstructionDefinition {
  std::string_view mnemonic;
  u8 base = 0;
  SharedOpBehavior behavior = SharedOpBehavior::INVALID;
  LowBitSpecifiers low_bits = LowBitSpecifiers::None;
  Register target = Register::INVALID;
};

constexpr u8 opcode_count(LowBitSpecifiers low_bits) {
  switch (low_bits) {
  case LowBitSpecifiers::None: return 1;
  case LowBitSpecifiers::A: return 2;
  case LowBitSpecifiers::AAA: [[fallthrough]];
  case LowBitSpecifiers::NNN: [[fallthrough]];
  case LowBitSpecifiers::RAAA: return 8;
  }
  return 0;
}

// Given an instruction's expected low_bits pattern and offset = (opcode-base), compute the addressing mode.
constexpr AddrMode low_bit_mode(LowBitSpecifiers low_bits, u8 offset) {
  switch (low_bits) {
  case LowBitSpecifiers::A: return offset == 0 ? AddrMode::I : AddrMode::X;
  case LowBitSpecifiers::AAA: [[fallthrough]];
  case LowBitSpecifiers::RAAA: return static_cast<AddrMode>(static_cast<u8>(AddrMode::I) + offset);
  default: return AddrMode::Unary;
  }
}

// Every defined mnemonic in the same order as rows.
inline std::vector<std::string_view> mnemonics(std::span<const InstructionDefinition> rows) {
  std::vector<std::string_view> ret;
  ret.reserve(rows.size());
  for (const auto &row : rows) ret.emplace_back(row.mnemonic);
  return ret;
}

// The base opcode in binary, with r for a register bit and a, aaa, or nnn for the low bits' field.
inline std::string specifier_with_placeholders(const InstructionDefinition &row) {
  auto ret = std::bitset<8>(row.base).to_string();
  const bool has_register = row.target == Register::A || row.target == Register::X;
  switch (row.low_bits) {
  case LowBitSpecifiers::None:
    if (has_register) ret[7] = 'r';
    break;
  case LowBitSpecifiers::A: ret[7] = 'a'; break;
  case LowBitSpecifiers::AAA: ret.replace(5, 3, "aaa"); break;
  case LowBitSpecifiers::NNN: ret.replace(5, 3, "nnn"); break;
  case LowBitSpecifiers::RAAA:
    ret.replace(5, 3, "aaa");
    ret[4] = 'r';
    break;
  }
  return ret;
}

// The row whose base+opcode_count include opcode. Nullptr if no match.
constexpr const InstructionDefinition *row_of(std::span<const InstructionDefinition> rows, u8 opcode) {
  for (const auto &row : rows)
    if (opcode >= row.base && opcode - row.base < opcode_count(row.low_bits)) return &row;
  return nullptr;
}

// Convert a number of InstructionDefinitions into 8-bit lookup table.
constexpr OpcodePlane make_opcode_plane(std::span<const InstructionDefinition> rows) {
  OpcodePlane plane{};
  for (const auto &row : rows) {
    if (row.base + opcode_count(row.low_bits) > plane.size()) throw std::out_of_range("Opcode exceeds 0xFF");
    for (u8 offset = 0; offset < opcode_count(row.low_bits); offset++) {
      auto &op = plane[row.base + offset];
      if (op.opcode != SharedOpBehavior::INVALID) throw std::logic_error("Overlapping opcodes");
      // Non-unary traps fetch the operand specifier as an immediate into OS. This advancs the PC by 2 automatically.
      auto mode = low_bit_mode(row.low_bits, offset);
      // Stores disallow immediate addressing. Keep the ,i variant INVALID.
      if (is_store(row.behavior) && mode == AddrMode::I) continue;
      if (is_trap(row.behavior)) mode = row.behavior == SharedOpBehavior::UNARY_TRAP ? AddrMode::Unary : AddrMode::I;
      op = Operation{.opcode = row.behavior, .addr_mode = mode, .target = row.target};
    }
  }
  return plane;
}
} // namespace pep
