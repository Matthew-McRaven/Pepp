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
#include "core/arch/pepx/asmb/pep10.hpp"
#include "core/arch/pepx/isa/pep10.hpp"

const pep::Dialect &pep::pep10::dialect() {
  using enum Directive;
  static const Dialect ret{.mnemonics = make_mnemonics(instructions),
                           .directives = mask_of({ALIGN, ASCII, BLOCK, BYTE, END, EQUATE, EXPORT, INPUT, ORG, OUTPUT,
                                                  SCALL, SECTION, WORD, IF, ELSEIF, ELSE, ENDIF, MACRO, ENDM})};
  return ret;
}
