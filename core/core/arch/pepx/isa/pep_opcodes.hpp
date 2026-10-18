/*
 * /Copyright (c) 2023-2026. Stanley Warford, Matthew McRaven
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
#include "core/architectures.hpp"
#include "core/arch/pepx/isa/pep_registers.hpp"
#include "core/integers.h"
namespace pep {
// The possible simulation behaviors of the pep CPU, formed by a union over all instructions of Pep8, 9, and 10.
enum class SharedOpBehavior : u8 {
  INVALID = 0,
  STOP,
  UNIMPL,
  RET,
  RETN,
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
  // Different from SCALL in that it must clear X register.
  NONUNARY_TRAP,
  // Like NONUNARY_TRAP, except OS is not fetched automatically.
  UNARY_TRAP,
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

// At the hardware level, the addressing modes for the instructions.
enum class AddrMode : u8 {
  Unary, // Unary
  I,     // Immediate
  D,     // Direct
  N,     // Indirect
  S,     // Stack
  SF,    // Stack deferred
  X,     // Indexed
  SX,    // Stack indexed
  SFX,   // Stack deferred indexed
  INVALID,
};

// Bit (1 << mode) for each AddrMode in a set.
using AddrModeMask = u16;
constexpr AddrModeMask mask_of(AddrMode mode) { return static_cast<AddrModeMask>(1u << static_cast<u8>(mode)); }

// How to interpret the low bits of the opcode?
enum class LowBitSpecifiers {
  None,
  A,
  AAA,
  NNN,
  RAAA,
};

using TargetRegister = Register;

struct Operation {
  SharedOpBehavior opcode = SharedOpBehavior::INVALID;
  AddrMode addr_mode = AddrMode::INVALID;
  TargetRegister target = Register::INVALID;
};

using OpcodePlane = std::array<Operation, 256>;

constexpr bool is_unary(Operation op) { return op.addr_mode == AddrMode::Unary; }
constexpr bool is_trap(SharedOpBehavior op) {
  return op == SharedOpBehavior::NONUNARY_TRAP || op == SharedOpBehavior::UNARY_TRAP || op == SharedOpBehavior::SCALL;
}
constexpr bool is_trap(Operation op) { return is_trap(op.opcode); }
// Calls and traps both push a return address, which matters to the debugger's step modes.
constexpr bool is_call(SharedOpBehavior op) { return op == SharedOpBehavior::CALL || is_trap(op); }
constexpr bool is_store(SharedOpBehavior op) { return op == SharedOpBehavior::STW || op == SharedOpBehavior::STB; }
constexpr u8 operand_width(Operation op) {
  if (is_unary(op)) return 0;
  switch (op.opcode) {
  case SharedOpBehavior::LDB:
  case SharedOpBehavior::CPB: return 1;
  default: return 2;
  }
}
constexpr bool operand_is_signed(Operation op) {
  using enum AddrMode;
  switch (op.addr_mode) {
  case I: [[fallthrough]];
  case S: [[fallthrough]];
  case SF: [[fallthrough]];
  case SX: [[fallthrough]];
  case SFX: return true;
  default: return false;
  }
}

}; // namespace pep