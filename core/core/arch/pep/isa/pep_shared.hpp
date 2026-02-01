#pragma once

#include <iostream>
#include <stdexcept>

/*
 * Copyright (c) 2023-2026 J. Stanley Warford, Matthew McRaven
 *
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

namespace isa::detail {

template <typename Mnemonic> uint8_t opcode(Mnemonic mnemonic) { return static_cast<uint8_t>(mnemonic); }

template <typename AddressingMode, typename Mnemonic> uint8_t opcode(Mnemonic mnemonic, AddressingMode addr) {
  using M = Mnemonic;
  using AM = AddressingMode;
  auto base = opcode(mnemonic);
  // TODO: Look up instruction type instead of doing opcode math.
  if (base >= static_cast<uint8_t>(M::BR) && base <= static_cast<uint8_t>(M::CALL))
    return base | (addr == AM::X ? 1 : 0);
  static const char *const e = "Invalid ADDR mode";
  switch (addr) {
  case AM::NONE: std::cerr << e; throw std::logic_error(e);
  case AM::I: return base | 0x0;
  case AM::D: return base | 0x1;
  case AM::N: return base | 0x2;
  case AM::S: return base | 0x3;
  case AM::SF: return base | 0x4;
  case AM::X: return base | 0x5;
  case AM::SX: return base | 0x6;
  case AM::SFX: return base | 0x7;
  case AM::ALL: std::cerr << e; throw std::logic_error(e);
  case AM::INVALID: std::cerr << e; throw std::logic_error(e);
  }
  static const char *const e2 = "Unreachable";
  std::cerr << e2;
  throw std::logic_error(e2);
}

template <typename Mnemonic> bool isStore(Mnemonic mnemonic) {
  using M = Mnemonic;
  if (mnemonic == M::STBA || mnemonic == M::STWA || mnemonic == M::STBX || mnemonic == M::STWX) return true;
  else return false;
}
} // namespace isa::detail
