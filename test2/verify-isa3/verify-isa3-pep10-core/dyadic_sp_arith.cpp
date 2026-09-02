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
  // Loop over a subset of possible values for the target register.
  u16 tmp;
  auto [init_reg] = GENERATE(table<u16>({0, 1, 0x7fff, 0x8000, 0x8FFF, 0xFFFF}));
  DYNAMIC_SECTION("with initial value " << init_reg) {
    for (uint16_t opspec = 0; static_cast<uint32_t>(opspec) + 1 < 0x1'0000; opspec++) {
      auto addedEnd = static_cast<u16>(opspec + init_reg);
      auto subedVal = static_cast<u16>((~opspec + 1) + init_reg);
      auto endRegVal = op == Mnemonic::ADDSP ? addedEnd : subedVal;

      // Object code for instruction under test.
      auto program =
          std::array<u8, 3>{(u8)op, static_cast<uint8_t>((opspec >> 8) & 0xff), static_cast<uint8_t>(opspec & 0xff)};

      cpu->registers()->clear(0);
      cpu->csrs()->clear(0);
      cpu->write_register(Register::SP, init_reg);

      REQUIRE_NOTHROW(mem->write(0, {program.data(), program.size()}, rw));
      REQUIRE_NOTHROW(cpu->clock_tick(PulseSchedule::PulseIndex{0}, 0));

      CHECK(reg(cpu, Register::A) == 0);
      CHECK(reg(cpu, Register::X) == 0);
      CHECK(reg(cpu, Register::PC) == 0x3);
      CHECK(reg(cpu, Register::IS) == (u8)op);
      // OS loaded the Mem[0x0001-0x0002].
      CHECK(reg(cpu, Register::OS) == opspec);
      // Check that target register had arithmetic performed.
      CHECK(reg(cpu, Register::SP) == endRegVal);
    }
  }
}

} // namespace

TEST_CASE("(new) Pep/10, ADDSP, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, MN::ADDSP);
}
TEST_CASE("(new) Pep/9, ADDSP, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep9::Register;
  using CSR = isa::Pep9::CSR;
  using MN = isa::Pep9::Mnemonic;
  inner<Register, CSR, MN>(PepISA3CPU::ISA::Pep9, MN::ADDSP);
}

TEST_CASE("(new) Pep/10, SUBSP, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, MN::SUBSP);
}
TEST_CASE("(new) Pep/9, SUBSP, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep9::Register;
  using CSR = isa::Pep9::CSR;
  using MN = isa::Pep9::Mnemonic;
  inner<Register, CSR, MN>(PepISA3CPU::ISA::Pep9, MN::SUBSP);
}
