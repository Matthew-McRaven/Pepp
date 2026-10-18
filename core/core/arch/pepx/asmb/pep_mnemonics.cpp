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
#include "core/arch/pepx/asmb/pep_mnemonics.hpp"

pep::MnemonicMap pep::make_mnemonics(std::span<const InstructionDefinition> rows) {
  using enum AddrMode;
  constexpr AddrModeMask all = mask_of(I) | mask_of(D) | mask_of(N) | mask_of(S) | mask_of(SF) | mask_of(X) |
                               mask_of(SX) | mask_of(SFX);
  MnemonicMap ret;
  for (const auto &row : rows) {
    const std::string name{row.mnemonic};
    const MnemonicDescriptor base{.base_opcode = row.base, .low_bits = row.low_bits};
    const auto with_mode = [&](AddrModeMask modes) {
      auto mn = base;
      mn.modes = modes;
      mn._operands = {MnemonicDescriptor::OperandType::Value, MnemonicDescriptor::OperandType::AddressingMode};
      return mn;
    };
    switch (row.low_bits) {
    case LowBitSpecifiers::None: {
      auto unary = base;
      unary.default_addr = Unary;
      ret.emplace(name, unary);
      break;
    }
    case LowBitSpecifiers::A: {
      auto implied = base;
      implied.default_addr = I;
      implied._operands = {MnemonicDescriptor::OperandType::Value};
      ret.emplace(name, implied);
      ret.emplace(name, with_mode(mask_of(I) | mask_of(X)));
      break;
    }
    case LowBitSpecifiers::AAA: [[fallthrough]];
    case LowBitSpecifiers::RAAA: {
      // All ISA variants for Pep disallow immediate addressing for stores.
      const auto modes = is_store(row.behavior) ? static_cast<AddrModeMask>(all & ~mask_of(I)) : all;
      ret.emplace(name, with_mode(modes));
      break;
    }
    case LowBitSpecifiers::NNN: throw std::logic_error("NNN mnemonics are not supported");
    }
  }
  return ret;
}
