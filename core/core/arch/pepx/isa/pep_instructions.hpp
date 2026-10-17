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
#include <span>
#include <stdexcept>
#include <string_view>
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

// Convert a number of InstructionDefinitions into 8-bit lookup table.
constexpr OpcodePlane make_opcode_plane(std::span<const InstructionDefinition> rows) {
  OpcodePlane plane{};
  for (const auto &row : rows) {
    if (row.base + opcode_count(row.low_bits) > plane.size()) throw std::out_of_range("Opcode exceeds 0xFF");
    for (u8 offset = 0; offset < opcode_count(row.low_bits); offset++) {
      auto &op = plane[row.base + offset];
      if (op.opcode != SharedOpBehavior::INVALID) throw std::logic_error("Overlapping opcodes");
      // Non-unary traps fetch the operand specifier as an immediate into OS. This advancs the PC by 2 automatically.
      auto mode = AddrMode::Unary;
      if (is_trap(row.behavior)) mode = row.behavior == SharedOpBehavior::UNARY_TRAP ? AddrMode::Unary : AddrMode::I;
      else {
        switch (row.low_bits) {
        case LowBitSpecifiers::A: mode = offset == 0 ? AddrMode::I : AddrMode::X; break;
        case LowBitSpecifiers::AAA: [[fallthrough]];
        case LowBitSpecifiers::RAAA: mode = static_cast<AddrMode>(static_cast<u8>(AddrMode::I) + offset); break;
        default: break;
        }
      }
      op = Operation{.opcode = row.behavior, .addr_mode = mode, .target = row.target};
    }
  }
  return plane;
}
} // namespace pep
