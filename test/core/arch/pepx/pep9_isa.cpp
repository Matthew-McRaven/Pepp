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
#include <catch.hpp>
#include <string>
#include <vector>
#include "core/arch/pep/isa/pep9.hpp"
#include "core/arch/pepx/asmb/pep9.hpp"
#include "core/arch/pepx/isa/pep9.hpp"

TEST_CASE("New Pep/9 architecture definition matches old definitions",
          "[scope:core][scope:core.arch][kind:unit][arch:pep9]") {
  using Old = isa::Pep9;
  using B = pep::SharedOpBehavior;
  SECTION("Mnemonics") {
    CHECK(pep::pep9::instructions.size() == Old::mnemonics().size());
    for (const auto &row : pep::pep9::instructions) {
      INFO(std::string{row.mnemonic});
      const auto mn = Old::parseMnemonic(std::string{row.mnemonic});
      REQUIRE(mn != Old::Mnemonic::INVALID);
      CHECK(Old::opcode(mn) == row.base);
    }
  }
  SECTION("Mnemonic list, help text, and call classification") {
    using Old = isa::Pep9;
    const auto names = pep::mnemonics(pep::pep9::instructions);
    CHECK(std::vector<std::string>(names.begin(), names.end()) == Old::mnemonics());
    for (const auto &row : pep::pep9::instructions) {
      INFO(std::string{row.mnemonic});
      const auto mn = Old::parseMnemonic(std::string{row.mnemonic});
      CHECK(pep::describe(pep::pep9::descriptions, row.mnemonic) == Old::describeMnemonicUsingPlaceholders(mn));
      // The old ISA leaves DECI's and STRO's mode field unmarked, though both encode one.
      auto expected = Old::instructionSpecifierWithPlaceholders(mn);
      if (row.mnemonic == "DECI" || row.mnemonic == "STRO") expected.replace(5, 3, "aaa");
      CHECK(pep::specifier_with_placeholders(row) == expected);
      CHECK(pep::is_call(row.behavior) == Old::isCall(mn));
      CHECK(pep::is_trap(row.behavior) == Old::isTrap(mn));
    }
  }
  SECTION("Opcode planes") {
    STATIC_REQUIRE(pep::pep9::opcode_plane[0x31].opcode == B::NONUNARY_TRAP);
    for (int it = 0; it < 256; it++) {
      INFO("opcode " << it);
      const auto &now = pep::pep9::opcode_plane[it];
      const auto &old = Old::opcode_plane[it];
      // Non-unary traps fetch their operand specifier as immediate, and the OS rejects modes the trap does not
      // support. The old plane decodes modes, and marks DECI's unsupported ones invalid.
      if (now.opcode == B::NONUNARY_TRAP) {
        CHECK(now.addr_mode == pep::AddrMode::I);
        CHECK((old.behavior == B::NONUNARY_TRAP || old.behavior == B::INVALID));
        continue;
      }
      CHECK(now.opcode == old.behavior);
      if (now.opcode == B::INVALID) continue;
      CHECK(now.addr_mode == old.addr);
      if (now.target != pep::Register::INVALID) CHECK(static_cast<u8>(now.target) == old.target);
    }
  }
}
