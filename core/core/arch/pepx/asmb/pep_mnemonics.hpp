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
#include <string>
#include <unordered_map>
#include "core/arch/pepx/isa/pep_instructions.hpp"
#include "core/arch/pepx/isa/pep_opcodes.hpp"
#include "core/ds/string_compare.hpp"
#include "core/integers.h"

namespace pep {

// A complete description of a single assembly-language mnemonic.
struct MnemonicDescriptor {
  enum class OperandType : u8 { None, Value, AddressingMode };
  // The byte for the first opcode of this mnemonic.
  u8 base_opcode = 0;
  // How many opcodes are used by this mnemonic, based on the number of low-bits used for the addressing mode.
  LowBitSpecifiers low_bits = LowBitSpecifiers::None;
  // Legal addressing modes.
  AddrModeMask modes = 0;
  // The mnemonic's default addressing mode if one is not provided.
  AddrMode default_addr = AddrMode::INVALID;
  // Ordered "arguments" of the mnemonic. e.g., ADDA would be a value and an addressing mode.
  // A value of None indicated the end of the list.
  std::array<OperandType, 2> _operands{};

  constexpr std::span<const OperandType> operands() const;
  constexpr bool allows(AddrMode mode) const;
  // The instruction specifier with mode packed into its low bits.
  constexpr u8 encode(AddrMode mode) const;
  constexpr bool operator==(const MnemonicDescriptor &) const = default;
};

// Valid patterns for each mnemonic. A mnemonic may have several patterns, e.g. BR v and BR v,i.
// The last pattern is treated as the canonical pattern for determining syntax errors.
using MnemonicMap = std::unordered_multimap<std::string, MnemonicDescriptor, pepp::bts::ci_hash, pepp::bts::ci_eq>;
MnemonicMap make_mnemonics(std::span<const InstructionDefinition> rows);

constexpr std::span<const MnemonicDescriptor::OperandType> MnemonicDescriptor::operands() const {
  std::size_t it = 0;
  for (; it < _operands.size(); it++)
    if (_operands[it] == OperandType::None) break;
  return {_operands.data(), it};
}

constexpr bool MnemonicDescriptor::allows(AddrMode mode) const { return (modes & mask_of(mode)) != 0; }

constexpr u8 MnemonicDescriptor::encode(AddrMode mode) const {
  switch (low_bits) {
  case LowBitSpecifiers::None: return base_opcode;
  case LowBitSpecifiers::A: return base_opcode | (mode == AddrMode::X ? 1 : 0);
  case LowBitSpecifiers::AAA: [[fallthrough]];
  case LowBitSpecifiers::RAAA: return base_opcode | (static_cast<u8>(mode) - static_cast<u8>(AddrMode::I));
  case LowBitSpecifiers::NNN: break;
  }
  throw std::logic_error("Low bits do not hold an addressing mode");
}
} // namespace pep
