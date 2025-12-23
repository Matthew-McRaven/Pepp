/*
 * Copyright (c) 2024 J. Stanley Warford, Matthew McRaven
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
#include "sim3/cores/pep/traced_pep9_isa3.hpp"
#include "./api.hpp"
#include "sim3/subsystems/ram/broadcast/mmo.hpp"
#include "sim3/subsystems/ram/dense.hpp"
#include "utils/bits/swap.hpp"

namespace {
void inner(isa::Pep9::Mnemonic op) {
  auto [mem, cpu] = make();
  using T = sim::memory::Output<quint16>;
  auto pwrOff = QSharedPointer<T>::create(
      sim::api2::device::Descriptor{.id = 1, .baseName = "pwrOff", .fullName = "/pwrOff"}, T::AddressSpan{0, 0});
  cpu->setPwrOff(pwrOff.get());
  auto endpoint = pwrOff->endpoint();
  CHECK(endpoint->at_end());
  // Object code for instruction under test.
  auto program = std::array<quint8, 1>{(quint8)op};

  cpu->regs()->clear(0);
  cpu->csrs()->clear(0);
  targets::isa::writeRegister<isa::Pep9>(cpu->regs(), isa::Pep9::Register::PC, 0, rw);
  REQUIRE_NOTHROW(mem->write(0x0000, {program.data(), program.size()}, rw));
  REQUIRE_NOTHROW(cpu->clock(0));
  CHECK(reg(cpu, isa::Pep9::Register::PC) == 1);
  CHECK(reg(cpu, isa::Pep9::Register::IS) == (quint8)op);
  CHECK(!endpoint->at_end());
}
} // namespace

TEST_CASE("Pep9::Mnemonic::STOP", "[scope:targets][kind:int][target:pep9]") { inner(isa::Pep9::Mnemonic::STOP); }
