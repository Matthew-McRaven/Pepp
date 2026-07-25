/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
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

namespace {

template <typename Register, typename CSR, typename Mnemonic> void inner(PepISA3CPU::ISA isa, Mnemonic op) {
  auto [sys, mem, cpu] = make_cpu(isa);
  // Object code for instruction under test.
  auto program = std::array<u8, 1>{(u8)op};
  REQUIRE_NOTHROW(mem->write(0, {program.data(), program.size()}, rw));

  cpu->registers()->clear(0);
  cpu->csrs()->clear(0);

  REQUIRE_NOTHROW(cpu->clock_tick(PulseSchedule::PulseIndex{0}, 0));

  CHECK(reg(cpu, Register::PC) == 0x1);
  CHECK(reg(cpu, Register::IS) == (u8)op);
  CHECK(reg(cpu, Register::OS) == 0);
  CHECK(reg(cpu, Register::X) == 0);
  CHECK(reg(cpu, Register::A) == 0);
}

} // namespace

TEST_CASE("(new) Pep/10, NOP, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, MN::NOP);
}
