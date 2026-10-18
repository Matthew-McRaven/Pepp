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

namespace pep::pep9 {
constexpr auto instructions = [] {
  using B = SharedOpBehavior;
  using L = LowBitSpecifiers;
  using R = Register;
  return std::to_array<InstructionDefinition>({
      {"STOP", 0x00, B::STOP},
      {"RET", 0x01, B::RET},
      {"RETTR", 0x02, B::SRET},
      {"MOVSPA", 0x03, B::MOVSPA},
      {"MOVFLGA", 0x04, B::MOVFLGA},
      {"MOVAFLG", 0x05, B::MOVAFLG},
      {"NOTA", 0x06, B::NOT, L::None, R::A},
      {"NOTX", 0x07, B::NOT, L::None, R::X},
      {"NEGA", 0x08, B::NEG, L::None, R::A},
      {"NEGX", 0x09, B::NEG, L::None, R::X},
      {"ASLA", 0x0A, B::ASL, L::None, R::A},
      {"ASLX", 0x0B, B::ASL, L::None, R::X},
      {"ASRA", 0x0C, B::ASR, L::None, R::A},
      {"ASRX", 0x0D, B::ASR, L::None, R::X},
      {"ROLA", 0x0E, B::ROL, L::None, R::A},
      {"ROLX", 0x0F, B::ROL, L::None, R::X},
      {"RORA", 0x10, B::ROR, L::None, R::A},
      {"RORX", 0x11, B::ROR, L::None, R::X},
      {"BR", 0x12, B::BR, L::A},
      {"BRLE", 0x14, B::BRLE, L::A},
      {"BRLT", 0x16, B::BRLT, L::A},
      {"BREQ", 0x18, B::BREQ, L::A},
      {"BRNE", 0x1A, B::BRNE, L::A},
      {"BRGE", 0x1C, B::BRGE, L::A},
      {"BRGT", 0x1E, B::BRGT, L::A},
      {"BRV", 0x20, B::BRV, L::A},
      {"BRC", 0x22, B::BRC, L::A},
      {"CALL", 0x24, B::CALL, L::A},
      {"NOP0", 0x26, B::UNARY_TRAP},
      {"NOP1", 0x27, B::UNARY_TRAP},
      {"NOP", 0x28, B::NONUNARY_TRAP, L::AAA},
      {"DECI", 0x30, B::NONUNARY_TRAP, L::AAA},
      {"DECO", 0x38, B::NONUNARY_TRAP, L::AAA},
      {"HEXO", 0x40, B::NONUNARY_TRAP, L::AAA},
      {"STRO", 0x48, B::NONUNARY_TRAP, L::AAA},
      {"ADDSP", 0x50, B::ADDSP, L::AAA, R::SP},
      {"SUBSP", 0x58, B::SUBSP, L::AAA, R::SP},
      {"ADDA", 0x60, B::ADD, L::RAAA, R::A},
      {"ADDX", 0x68, B::ADD, L::RAAA, R::X},
      {"SUBA", 0x70, B::SUB, L::RAAA, R::A},
      {"SUBX", 0x78, B::SUB, L::RAAA, R::X},
      {"ANDA", 0x80, B::AND, L::RAAA, R::A},
      {"ANDX", 0x88, B::AND, L::RAAA, R::X},
      {"ORA", 0x90, B::OR, L::RAAA, R::A},
      {"ORX", 0x98, B::OR, L::RAAA, R::X},
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

// Prebuilt at compile time.
constexpr OpcodePlane opcode_plane = []() consteval { return make_opcode_plane(instructions); }();

enum class MemoryVectors : u16 {
  UserStackPtr = 0xFFF4,
  SystemStackPtr = 0xFFF6,
  CharIn = 0xFFF8,
  CharOut = 0xFFFA,
  Loader = 0xFFFC,
  TrapHandler = 0xFFFE,
};
} // namespace pep::pep9
