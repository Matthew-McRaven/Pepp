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

template <typename Register, typename CSR, typename Mnemonic>
void inner_ldw(PepISA3CPU::ISA isa, Register target_reg, Mnemonic op) {
  auto [sys, mem, cpu] = make_cpu(isa);
  // Loop over a subset of possible values for the target register.
  for (uint16_t opspec = 0; static_cast<uint32_t>(opspec) + 1 < 0x1'0000; opspec++) {

    // Object code for instruction under test.
    auto program =
        std::array<u8, 3>{(u8)op, static_cast<uint8_t>((opspec >> 8) & 0xff), static_cast<uint8_t>(opspec & 0xff)};

    cpu->registers()->clear(0);
    cpu->csrs()->clear(0);

    REQUIRE_NOTHROW(mem->write(0, {program.data(), program.size()}, rw));
    REQUIRE_NOTHROW(cpu->clock_tick(PulseSchedule::PulseIndex{0}, 0));

    CHECK(reg(cpu, Register::SP) == 0);
    CHECK(reg(cpu, Register::PC) == 0x3);
    CHECK(reg(cpu, Register::IS) == (u8)op);
    // OS loaded the Mem[0x0001-0x0002].
    CHECK(reg(cpu, Register::OS) == opspec);
    // Check that target register had arithmetic performed.
    CHECK(reg(cpu, target_reg) == opspec);
    // Check that target status bits match RTL.
    CHECK(csr(cpu, CSR::N) == (opspec & 0x8000 ? 1 : 0));
    CHECK(!!csr(cpu, CSR::Z) == (opspec == 0));
  }
}

template <typename Register, typename CSR, typename Mnemonic>
void inner_ldb(PepISA3CPU::ISA isa, Register target_reg, Mnemonic op) {
  auto [sys, mem, cpu] = make_cpu(isa);
  // Loop over a subset of possible values for the target register.
  for (uint16_t opspec = 0; static_cast<uint32_t>(opspec) + 1 < 0x1'00; opspec++) {

    // Object code for instruction under test.
    auto program = std::array<u8, 3>{(u8)op, (u8)0, static_cast<uint8_t>(opspec & 0xff)};

    cpu->registers()->clear(0);
    cpu->csrs()->clear(0);

    REQUIRE_NOTHROW(mem->write(0, {program.data(), program.size()}, rw));
    REQUIRE_NOTHROW(cpu->clock_tick(PulseSchedule::PulseIndex{0}, 0));

    CHECK(reg(cpu, Register::SP) == 0);
    CHECK(reg(cpu, Register::PC) == 0x3);
    CHECK(reg(cpu, Register::IS) == (u8)op);
    // OS loaded the Mem[0x0001-0x0002].
    CHECK(reg(cpu, Register::OS) == opspec);
    // Check that target register had arithmetic performed.
    CHECK(reg(cpu, target_reg) == opspec);
    // Check that target status bits match RTL.
    CHECK(csr(cpu, CSR::N) == 0);
    CHECK(csr(cpu, CSR::Z) == (opspec == 0));
  }
}

} // namespace

TEST_CASE("(new) Pep/10, LDWA, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner_ldw<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, Register::A, MN::LDWA);
}
TEST_CASE("(new) Pep/9, LDWA, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep9::Register;
  using CSR = isa::Pep9::CSR;
  using MN = isa::Pep9::Mnemonic;
  inner_ldw<Register, CSR, MN>(PepISA3CPU::ISA::Pep9, Register::A, MN::LDWA);
}
TEST_CASE("(new) Pep/10, LDWX, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner_ldw<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, Register::X, MN::LDWX);
}
TEST_CASE("(new) Pep/9, LDWX, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep9::Register;
  using CSR = isa::Pep9::CSR;
  using MN = isa::Pep9::Mnemonic;
  inner_ldw<Register, CSR, MN>(PepISA3CPU::ISA::Pep9, Register::X, MN::LDWX);
}

TEST_CASE("(new) Pep/10, LDBA, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner_ldb<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, Register::A, MN::LDBA);
}
TEST_CASE("(new) Pep/9, LDBA, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep9::Register;
  using CSR = isa::Pep9::CSR;
  using MN = isa::Pep9::Mnemonic;
  inner_ldb<Register, CSR, MN>(PepISA3CPU::ISA::Pep9, Register::A, MN::LDBA);
}
TEST_CASE("(new) Pep/10, LDBX, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner_ldb<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, Register::X, MN::LDBX);
}
TEST_CASE("(new) Pep/9, LDBX, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep9::Register;
  using CSR = isa::Pep9::CSR;
  using MN = isa::Pep9::Mnemonic;
  inner_ldb<Register, CSR, MN>(PepISA3CPU::ISA::Pep9, Register::X, MN::LDBX);
}
