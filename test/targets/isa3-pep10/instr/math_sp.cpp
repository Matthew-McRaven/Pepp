/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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

#include <catch.hpp>

#include "sim3/cores/pep/traced_helpers.hpp"
#include "./api.hpp"
#include "sim3/subsystems/ram/dense.hpp"
#include "sim3/cores/pep/traced_pep10_isa3.hpp"
#include "bts/bitmanip/swap.hpp"

namespace {
void inner(isa::Pep10::Mnemonic op) {
  auto [mem, cpu] = make();
  // Loop over a subset of possible values for the target register.
  quint16 tmp;
  auto [init_reg] = GENERATE(table<quint16>({0, 1, 0x7fff, 0x8000, 0x8FFF, 0xFFFF}));
  DYNAMIC_SECTION("with initial value " << init_reg) {
    for (uint16_t opspec = 0; static_cast<uint32_t>(opspec) + 1 < 0x1'0000; opspec++) {
      auto addedEnd = static_cast<quint16>(opspec + init_reg);
      auto subedVal = static_cast<quint16>((~opspec + 1) + init_reg);
      auto endRegVal = op == isa::Pep10::Mnemonic::ADDSP ? addedEnd : subedVal;

      // Object code for instruction under test.
      auto program = std::array<quint8, 3>{(quint8)op, static_cast<uint8_t>((opspec >> 8) & 0xff),
                                           static_cast<uint8_t>(opspec & 0xff)};

      cpu->regs()->clear(0);
      cpu->csrs()->clear(0);
      tmp = bits::hostOrder() != bits::Order::BigEndian ? bits::byteswap(init_reg) : init_reg;
      cpu->regs()->write(static_cast<quint16>(isa::Pep10::Register::SP) * 2, {reinterpret_cast<quint8 *>(&tmp), 2}, rw);

      REQUIRE_NOTHROW(mem->write(0, {program.data(), program.size()}, rw));
      REQUIRE_NOTHROW(cpu->clock(0));

      CHECK(reg(cpu, isa::Pep10::Register::PC) == 0x3);
      CHECK(reg(cpu, isa::Pep10::Register::IS) == (quint8)op);
      // OS loaded the Mem[0x0001-0x0002].
      CHECK(reg(cpu, isa::Pep10::Register::OS) == opspec);
      CHECK(reg(cpu, isa::Pep10::Register::SP) == endRegVal);
    }
  }
}
} // namespace

TEST_CASE("ADDSP, i", "[scope:targets][kind:int][target:pep10]") { inner(isa::Pep10::Mnemonic::ADDSP); }
TEST_CASE("SUBSP, i", "[scope:targets][kind:int][target:pep10]") { inner(isa::Pep10::Mnemonic::SUBSP); }
