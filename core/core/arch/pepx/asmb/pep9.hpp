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
#include "core/arch/pepx/asmb/pep_dialect.hpp"
#include "core/arch/pepx/asmb/pep_mnemonics.hpp"

namespace pep::pep9 {
// Help text for each mnemonic.
constexpr auto descriptions = std::to_array<MnemonicDescription>({
    {"STOP", "Stop execution"},
    {"RET", "Return from CALL"},
    {"RETTR", "Return from trap"},
    {"MOVSPA", "Move SP to A"},
    {"MOVFLGA", "Move NZVC flags to A<12..15>"},
    {"MOVAFLG", "Move A<12..15> to NZVC flags"},
    {"NOTA", "Bitwise invert r"},
    {"NOTX", "Bitwise invert r"},
    {"NEGA", "Negate r"},
    {"NEGX", "Negate r"},
    {"ASLA", "Arithmetic shift left r"},
    {"ASLX", "Arithmetic shift left r"},
    {"ASRA", "Arithmetic shift right r"},
    {"ASRX", "Arithmetic shift right r"},
    {"ROLA", "Rotate left r"},
    {"ROLX", "Rotate left r"},
    {"RORA", "Rotate right r"},
    {"RORX", "Rotate right r"},
    {"BR", "Branch unconditional"},
    {"BRLE", "Branch if less than or equal to"},
    {"BRLT", "Branch if less than"},
    {"BREQ", "Branch if equal to"},
    {"BRNE", "Branch if not equal to"},
    {"BRGE", "Branch if greater than or equal to"},
    {"BRGT", "Branch if greater than"},
    {"BRV", "Branch if V"},
    {"BRC", "Branch if C"},
    {"CALL", "Call subroutine"},
    {"NOP0", "Unary no operation trap"},
    {"NOP1", "Unary no operation trap"},
    {"NOP", "Nonunary no operation trap"},
    {"DECI", "Decimal input trap"},
    {"DECO", "Decimal output trap"},
    {"HEXO", "Hexadecimal output trap"},
    {"STRO", "String output trap"},
    {"ADDSP", "Add to stack pointer (SP)"},
    {"SUBSP", "Subtract from stack pointer (SP)"},
    {"ADDA", "Add to r"},
    {"ADDX", "Add to r"},
    {"SUBA", "Subtract from r"},
    {"SUBX", "Subtract from r"},
    {"ANDA", "Bitwise AND to r"},
    {"ANDX", "Bitwise AND to r"},
    {"ORA", "Bitwise OR to r"},
    {"ORX", "Bitwise OR to r"},
    {"CPWA", "Compare word to r"},
    {"CPWX", "Compare word to r"},
    {"CPBA", "Compare byte to r<8..15>"},
    {"CPBX", "Compare byte to r<8..15>"},
    {"LDWA", "Load word r from memory"},
    {"LDWX", "Load word r from memory"},
    {"LDBA", "Load byte r<8..15> from memory"},
    {"LDBX", "Load byte r<8..15> from memory"},
    {"STWA", "Store word r to memory"},
    {"STWX", "Store word r to memory"},
    {"STBA", "Store byte r<8..15> to memory"},
    {"STBX", "Store byte r<8..15> to memory"},
});

const Dialect &dialect();
} // namespace pep::pep9
