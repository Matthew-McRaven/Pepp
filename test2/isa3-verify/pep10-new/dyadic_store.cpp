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
 * You shoust have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <catch.hpp>
#include "./api.hpp"

namespace {

template <typename Register, typename CSR, typename Mnemonic>
void inner_st(PepISA3CPU::ISA isa, Register target_reg, Mnemonic op, u8 length) {
  auto [sys, mem, cpu] = make_cpu(isa);

  // Loop over a subset of possible values for the target register.
  for (uint16_t opspec = 0; static_cast<uint32_t>(opspec) + 2 < 0x1'0000; opspec++) {

    // Object code for instruction under test.
    auto program = std::array<u8, 3>{(u8)((u8)op + 1), // offset by 1 to get direct addressing.
                                     static_cast<u8>((opspec >> 8) & 0xff), static_cast<uint8_t>(opspec & 0xff)};

    cpu->registers()->clear(0);
    cpu->csrs()->clear(0);
    cpu->write_register(target_reg, 0xfeed);

    REQUIRE_NOTHROW(mem->write(0, {program.data(), program.size()}, rw));
    REQUIRE_NOTHROW(cpu->clock_tick(PulseSchedule::PulseIndex{0}, 0));

    CHECK(reg(cpu, Register::SP) == 0);
    CHECK(reg(cpu, Register::PC) == 0x3);
    CHECK(reg(cpu, Register::IS) == (i16)(1 + (u8)op));
    // OS loaded the Mem[0x0001-0x0002].
    CHECK(reg(cpu, Register::OS) == opspec);
    // Memory at opspec was written
    if (length == 2) {
      auto v = ((Target *)mem)->read<u16, bits::host_is_le>(opspec, rw).second;
      CHECK(v == 0xfeed);
    } else if (length == 1) {
      auto v = ((Target *)mem)->read<u8>(opspec, rw).second;
      CHECK(v == 0xed);
    }
  }
}

} // namespace

TEST_CASE("(new) Pep/10, STWA, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner_st<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, Register::A, MN::STWA, 2);
}
TEST_CASE("(new) Pep/9, STWA, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep9::Register;
  using CSR = isa::Pep9::CSR;
  using MN = isa::Pep9::Mnemonic;
  inner_st<Register, CSR, MN>(PepISA3CPU::ISA::Pep9, Register::A, MN::STWA, 2);
}
TEST_CASE("(new) Pep/10, STWX, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner_st<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, Register::X, MN::STWX, 2);
}
TEST_CASE("(new) Pep/9, STWX, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep9::Register;
  using CSR = isa::Pep9::CSR;
  using MN = isa::Pep9::Mnemonic;
  inner_st<Register, CSR, MN>(PepISA3CPU::ISA::Pep9, Register::X, MN::STWX, 2);
}

TEST_CASE("(new) Pep/10, STBA, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner_st<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, Register::A, MN::STBA, 1);
}
TEST_CASE("(new) Pep/9, STBA, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep9::Register;
  using CSR = isa::Pep9::CSR;
  using MN = isa::Pep9::Mnemonic;
  inner_st<Register, CSR, MN>(PepISA3CPU::ISA::Pep9, Register::A, MN::STBA, 1);
}
TEST_CASE("(new) Pep/10, STBX, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner_st<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, Register::X, MN::STBX, 1);
}
TEST_CASE("(new) Pep/9, STBX, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep9::Register;
  using CSR = isa::Pep9::CSR;
  using MN = isa::Pep9::Mnemonic;
  inner_st<Register, CSR, MN>(PepISA3CPU::ISA::Pep9, Register::X, MN::STBX, 1);
}
