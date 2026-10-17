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
#include <stdexcept>
#include <string>
#include <vector>
#include "core/arch/pep/isa/pep10.hpp"
#include "core/arch/pepx/isa/pep10.hpp"

TEST_CASE("New Pep/10 architeture definition matches old definitions") {
  SECTION("Number of mnemonics") {
    using Old = isa::Pep10;
    CHECK(pep::pep10::instructions.size() == Old::mnemonics().size());
    for (const auto &row : pep::pep10::instructions) {
      INFO(std::string{row.mnemonic});
      const auto mn = Old::parseMnemonic(std::string{row.mnemonic});
      REQUIRE(mn != Old::Mnemonic::INVALID);
      CHECK(Old::opcode(mn) == row.base);
    }
  }
  SECTION("Register numbers") {
    CHECK(static_cast<u8>(pep::Register::A) == static_cast<u8>(isa::Pep10::Register::A));
    CHECK(static_cast<u8>(pep::Register::X) == static_cast<u8>(isa::Pep10::Register::X));
    CHECK(static_cast<u8>(pep::Register::SP) == static_cast<u8>(isa::Pep10::Register::SP));
    CHECK(static_cast<u8>(pep::Register::PC) == static_cast<u8>(isa::Pep10::Register::PC));
    CHECK(static_cast<u8>(pep::Register::IS) == static_cast<u8>(isa::Pep10::Register::IS));
    CHECK(static_cast<u8>(pep::Register::OS) == static_cast<u8>(isa::Pep10::Register::OS));
    CHECK(static_cast<u8>(pep::StatusBit::N) == static_cast<u8>(isa::Pep10::CSR::N));
  CHECK(static_cast<u8>(pep::StatusBit::Z) == static_cast<u8>(isa::Pep10::CSR::Z));
  CHECK(static_cast<u8>(pep::StatusBit::V) == static_cast<u8>(isa::Pep10::CSR::V));
  CHECK(static_cast<u8>(pep::StatusBit::C) == static_cast<u8>(isa::Pep10::CSR::C));
}
  SECTION("Opcode planes") {
    using B = pep::SharedOpBehavior;
    STATIC_REQUIRE(pep::pep10::opcode_plane[0xC5].opcode == B::LDW);
    for (int it = 0; it < 256; it++) {
      INFO("opcode " << it);
      const auto &now = pep::pep10::opcode_plane[it];
      const auto &old = isa::Pep10::opcode_plane[it];
      // The new plane keeps immediate stores; their handler must reject them.
      if (pep::is_store(now.opcode) && now.addr_mode == pep::AddrMode::I) {
        CHECK(old.behavior == B::INVALID);
        continue;
      }
      CHECK(now.opcode == old.behavior);
      if (now.opcode == B::INVALID || now.opcode == B::UNIMPL) continue;
      // The old plane marks SCALL unary; the new one fetches its operand specifier as immediate.
      if (now.opcode == B::SCALL) CHECK(now.addr_mode == pep::AddrMode::I);
      else CHECK(now.addr_mode == old.addr);
      // The old plane spells "no register" as either -1 or Register::INVALID.
      if (now.target != pep::Register::INVALID) CHECK(static_cast<u8>(now.target) == old.target);
    }
  }
}
