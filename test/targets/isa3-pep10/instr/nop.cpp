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

#include "./api.hpp"
#include "core/math/bitmanip/swap.hpp"
#include "sim3/cores/pep/traced_helpers.hpp"
#include "sim3/cores/pep/traced_pep10_isa3.hpp"
#include "sim3/subsystems/ram/dense.hpp"

TEST_CASE("NOP", "[scope:targets][kind:int][target:pep10]") {
  using Register = isa::Pep10::Register;
  auto [mem, cpu] = make();
  quint16 tmp;
  cpu->regs()->clear(0);
  cpu->csrs()->clear(0);

  auto program = std::array<quint8, 1>{(quint8)isa::Pep10::Mnemonic::NOP};
  REQUIRE_NOTHROW(mem->write(0, {program.data(), program.size()}, rw));
  REQUIRE_NOTHROW(cpu->clock(0));

  CHECK(reg(cpu, Register::SP) == 0);
  CHECK(reg(cpu, Register::PC) == 0x1);
  CHECK(reg(cpu, Register::IS) == (quint8)isa::Pep10::Mnemonic::NOP);
  CHECK(reg(cpu, Register::A) == 0);
  CHECK(reg(cpu, Register::X) == 0);
}
