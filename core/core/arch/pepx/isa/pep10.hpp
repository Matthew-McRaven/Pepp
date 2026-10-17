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
#include "core/arch/pepx/isa/pep_instructions.hpp"
#include "core/arch/pepx/isa/pep_opcodes.hpp"
#include "core/arch/pepx/isa/pep_registers.hpp"
#include "core/integers.h"

namespace pep::pep10 {
constexpr auto instructions = [] {
  using B = SharedOpBehavior;
  using L = LowBitSpecifiers;
  using R = Register;
  return std::to_array<InstructionDefinition>({
      {"RET", 0x01, B::RET},
      {"SRET", 0x02, B::SRET},
      {"MOVFLGA", 0x03, B::MOVFLGA},
      {"MOVAFLG", 0x04, B::MOVAFLG},
      {"MOVSPA", 0x05, B::MOVSPA},
      {"MOVASP", 0x06, B::MOVASP},
      {"NOP", 0x07, B::HW_NOP},
      {"NEGA", 0x18, B::NEG, L::None, R::A},
      {"NEGX", 0x19, B::NEG, L::None, R::X},
      {"ASLA", 0x1A, B::ASL, L::None, R::A},
      {"ASLX", 0x1B, B::ASL, L::None, R::X},
      {"ASRA", 0x1C, B::ASR, L::None, R::A},
      {"ASRX", 0x1D, B::ASR, L::None, R::X},
      {"NOTA", 0x1E, B::NOT, L::None, R::A},
      {"NOTX", 0x1F, B::NOT, L::None, R::X},
      {"ROLA", 0x20, B::ROL, L::None, R::A},
      {"ROLX", 0x21, B::ROL, L::None, R::X},
      {"RORA", 0x22, B::ROR, L::None, R::A},
      {"RORX", 0x23, B::ROR, L::None, R::X},
      {"BR", 0x24, B::BR, L::A},
      {"BRLE", 0x26, B::BRLE, L::A},
      {"BRLT", 0x28, B::BRLT, L::A},
      {"BREQ", 0x2A, B::BREQ, L::A},
      {"BRNE", 0x2C, B::BRNE, L::A},
      {"BRGE", 0x2E, B::BRGE, L::A},
      {"BRGT", 0x30, B::BRGT, L::A},
      {"BRV", 0x32, B::BRV, L::A},
      {"BRC", 0x34, B::BRC, L::A},
      {"CALL", 0x36, B::CALL, L::A},
      {"SCALL", 0x38, B::SCALL, L::AAA},
      {"ADDSP", 0x40, B::ADDSP, L::AAA},
      {"SUBSP", 0x48, B::SUBSP, L::AAA},
      {"ADDA", 0x50, B::ADD, L::RAAA, R::A},
      {"ADDX", 0x58, B::ADD, L::RAAA, R::X},
      {"SUBA", 0x60, B::SUB, L::RAAA, R::A},
      {"SUBX", 0x68, B::SUB, L::RAAA, R::X},
      {"ANDA", 0x70, B::AND, L::RAAA, R::A},
      {"ANDX", 0x78, B::AND, L::RAAA, R::X},
      {"ORA", 0x80, B::OR, L::RAAA, R::A},
      {"ORX", 0x88, B::OR, L::RAAA, R::X},
      {"XORA", 0x90, B::XOR, L::RAAA, R::A},
      {"XORX", 0x98, B::XOR, L::RAAA, R::X},
      {"CPWA", 0xA0, B::CPW, L::RAAA, R::A},
      {"CPWX", 0xA8, B::CPW, L::RAAA, R::X},
      {"CPBA", 0xB0, B::CPB, L::RAAA, R::A},
      {"CPBX", 0xB8, B::CPB, L::RAAA, R::X},
      {"LDWA", 0xC0, B::LDW, L::RAAA, R::A},
      {"LDWX", 0xC8, B::LDW, L::RAAA, R::X},
      {"LDBA", 0xD0, B::LDB, L::RAAA, R::A},
      {"LDBX", 0xD8, B::LDB, L::RAAA, R::X},
      {"STWA", 0xE0, B::STW, L::RAAA, R::A},
      {"STWX", 0xE8, B::STW, L::RAAA, R::X},
      {"STBA", 0xF0, B::STB, L::RAAA, R::A},
      {"STBX", 0xF8, B::STB, L::RAAA, R::X},
  });
}();

// Prebuilt at compile time; make_opcode_plane also runs on rows built at runtime.
constexpr OpcodePlane opcode_plane = []() consteval {
  auto plane = make_opcode_plane(instructions);
  // Reserved for future instructions
  for (int it = 0x08; it < 0x18; it++)
    plane[it] = Operation{.opcode = SharedOpBehavior::UNIMPL, .addr_mode = AddrMode::Unary};
  return plane;
}();
} // namespace pep::pep10
