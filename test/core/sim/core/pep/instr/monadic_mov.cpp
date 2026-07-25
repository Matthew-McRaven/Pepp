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

template <typename Register, typename CSR, typename Mnemonic> void inner_movflg(PepISA3CPU::ISA isa, Mnemonic op) {
  auto [sys, mem, cpu] = make_cpu(isa);
  // Object code for instruction under test.
  auto program = std::array<u8, 1>{(u8)op};
  REQUIRE_NOTHROW(mem->write(0, {program.data(), program.size()}, rw));
  // Loop over a subset of possible values for the target register.
  u16 tmp;
  for (u16 flg = 0; flg < 0b1'00'00; flg++) {

    cpu->registers()->clear(0);
    cpu->csrs()->clear(0);

    if (op == Mnemonic::MOVAFLG) cpu->write_register(Register::A, flg);
    else cpu->write_packed_csr(flg);

    REQUIRE_NOTHROW(cpu->clock_tick(PulseSchedule::PulseIndex{0}, 0));

    CHECK(reg(cpu, Register::PC) == 0x1);
    CHECK(reg(cpu, Register::IS) == (u8)op);
    CHECK(reg(cpu, Register::OS) == 0);
    CHECK(reg(cpu, Register::X) == 0);
    // Check that target register had arithmetic performed.
    CHECK(reg(cpu, Register::A) == flg);
    CHECK(csr(cpu, CSR::N) == (flg & 0b1000 ? 1 : 0));
    CHECK(csr(cpu, CSR::Z) == (flg & 0b0100 ? 1 : 0));
    CHECK(csr(cpu, CSR::V) == (flg & 0b0010 ? 1 : 0));
    CHECK(csr(cpu, CSR::C) == (flg & 0b0001 ? 1 : 0));
  }
}

template <typename Register, typename CSR, typename Mnemonic>
void inner_movr(PepISA3CPU::ISA isa, Register src_reg, Register dest_reg, Mnemonic op) {
  auto [sys, mem, cpu] = make_cpu(isa);
  // Loop over a subset of possible values for the target register.
  u16 tmp;
  auto [init_reg] = GENERATE(table<u16>({0, 1, 0x7fff, 0x8000, 0x8FFF, 0xFFFF}));
  DYNAMIC_SECTION("with initial value " << init_reg) {

    // Object code for instruction under test.
    auto program = std::array<u8, 1>{(u8)op};

    cpu->registers()->clear(0);
    cpu->csrs()->clear(0);
    cpu->write_register(src_reg, init_reg);

    REQUIRE_NOTHROW(mem->write(0, {program.data(), program.size()}, rw));
    REQUIRE_NOTHROW(cpu->clock_tick(PulseSchedule::PulseIndex{0}, 0));

    CHECK(reg(cpu, src_reg) == init_reg);
    CHECK(reg(cpu, Register::PC) == 0x1);
    CHECK(reg(cpu, Register::IS) == (u8)op);
    // Register was copied
    CHECK(reg(cpu, dest_reg) == init_reg);
  }
}

} // namespace

TEST_CASE("(new) Pep/10, MOVFLGA, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner_movflg<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, MN::MOVFLGA);
}
TEST_CASE("(new) Pep/9, MOVFLGA, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep9::Register;
  using CSR = isa::Pep9::CSR;
  using MN = isa::Pep9::Mnemonic;
  inner_movflg<Register, CSR, MN>(PepISA3CPU::ISA::Pep9, MN::MOVFLGA);
}
TEST_CASE("(new) Pep/10, MOVAFLG, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner_movflg<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, MN::MOVAFLG);
}
TEST_CASE("(new) Pep/9, MOVAFLG, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep9::Register;
  using CSR = isa::Pep9::CSR;
  using MN = isa::Pep9::Mnemonic;
  inner_movflg<Register, CSR, MN>(PepISA3CPU::ISA::Pep9, MN::MOVAFLG);
}

TEST_CASE("(new) Pep/10, MOVSPA, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner_movr<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, Register::SP, Register::A, MN::MOVSPA);
}
TEST_CASE("(new) Pep/9, MOVSPA, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep9::Register;
  using CSR = isa::Pep9::CSR;
  using MN = isa::Pep9::Mnemonic;
  inner_movr<Register, CSR, MN>(PepISA3CPU::ISA::Pep9, Register::SP, Register::A, MN::MOVSPA);
}

TEST_CASE("(new) Pep/10, MOVASP, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner_movr<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, Register::A, Register::SP, MN::MOVASP);
}
