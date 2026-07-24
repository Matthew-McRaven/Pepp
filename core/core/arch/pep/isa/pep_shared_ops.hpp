#pragma once

#include <iostream>
#include <stdexcept>
#include "core/integers.h"

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

namespace isa {

// The possible simulation behaviors of the pep CPU, formed by a union over all instructions of Pep8, 9, and 10.
enum class SharedOpBehavior : u8 {
  INVALID = 0,
  STOP,
  UNIMPL,
  RET,
  SRET, // Covers RETTR and SRET
  MOVFLGA,
  MOVAFLG,
  MOVSPA,
  MOVASP,
  HW_NOP,
  NEG,
  ASL,
  ASR,
  NOT,
  ROL,
  ROR,
  BR,
  BRLE,
  BRLT,
  BREQ,
  BRNE,
  BRGE,
  BRGT,
  BRV,
  BRC,
  CALL,
  SCALL,
  TRAP_CALL, // Different from SCALL in that it must clear X register.
  ADDSP,
  SUBSP,
  ADD,
  SUB,
  AND,
  OR,
  XOR,
  CPW,
  CPB,
  LDW,
  LDB,
  STW,
  STB,
  // Must be last, by convention!
  MAX_VALUE
};
enum class SharedAddrMode : u8 {
  Unary, // Unary
  I,     // Immediate
  D,     // Direct
  N,     // Indirect
  S,     // Stack
  SF,    // Stack deferred
  X,     // Indexed
  SX,    // Stack indexed
  SFX,   // Stack deferred indexed
};

struct SharedOp {
  SharedOpBehavior behavior;
  SharedAddrMode addr;
  // Use your arch-specific enum value here. -1 is a placeholder for None.
  u8 target = static_cast<u8>(-1);
};

using OpcodePlane = std::array<SharedOp, 256>;
} // namespace isa
