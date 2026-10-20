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
#include "core/arch/pepx/asmb/pep9.hpp"
#include <string_view>
#include <utility>
#include "core/arch/pepx/isa/pep9.hpp"

const pep::Dialect &pep::pep9::dialect() {
  static const Dialect ret = [] {
    auto mnemonics = make_mnemonics(instructions);
    // The OS's trap handler rejects the other modes, so the assembler does too.
    const auto allow = [&](std::string_view name, AddrModeMask modes) {
      for (auto [it, end] = mnemonics.equal_range(name); it != end; ++it) it->second.modes = modes;
    };
    using enum AddrMode;
    // Patch the allowed addressing modes for the NONUNARY trap mnemonics
    allow("NOP", mask_of(I));
    allow("DECI", mask_of(D) | mask_of(N) | mask_of(S) | mask_of(SF) | mask_of(X) | mask_of(SX) | mask_of(SFX));
    allow("STRO", mask_of(D) | mask_of(N) | mask_of(S) | mask_of(SF) | mask_of(X));
    using enum Directive;
    // The book's set, plus EXPORT/INPUT/OUTPUT so the OS can declare its exports and MMIO.
    const auto directives = mask_of({ADDRSS, ALIGN, ASCII, BLOCK, BURN, BYTE, END, EQUATE, WORD, EXPORT, INPUT, OUTPUT});
    return Dialect{.mnemonics = std::move(mnemonics), .directives = directives};
  }();
  return ret;
}
