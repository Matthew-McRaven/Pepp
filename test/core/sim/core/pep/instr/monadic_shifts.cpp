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
void inner_asl(PepISA3CPU::ISA isa, Register target_reg, Register other_reg, Mnemonic op) {
  auto [sys, mem, cpu] = make_cpu(isa);
  // Loop over a subset of possible values for the target register.
  u16 tmp;
  auto [init_reg] = GENERATE(table<u16>({0, 1, 0x7fff, 0x8000, 0x8FFF, 0xFFFF}));
  DYNAMIC_SECTION("with initial value " << init_reg) {
    auto endRegVal = static_cast<u16>(init_reg << 1);

    // Object code for instruction under test.
    auto program = std::array<u8, 1>{(u8)op};

    cpu->registers()->clear(0);
    cpu->csrs()->clear(0);
    cpu->write_register(target_reg, init_reg);

    REQUIRE_NOTHROW(mem->write(0, {program.data(), program.size()}, rw));
    REQUIRE_NOTHROW(cpu->clock_tick(PulseSchedule::PulseIndex{0}, 0));

    CHECK(reg(cpu, Register::SP) == 0);
    CHECK(reg(cpu, other_reg) == 0);
    CHECK(reg(cpu, Register::PC) == 0x1);
    CHECK(reg(cpu, Register::IS) == (u8)op);
    // Check that target register had arithmetic performed.
    CHECK(reg(cpu, target_reg) == endRegVal);

    // Check that target register had arithmetic performed.
    CHECK(reg(cpu, target_reg) == endRegVal);
    // Check that target status bits match RTL.
    CHECK(csr(cpu, CSR::N) == (endRegVal & 0x8000 ? 1 : 0));
    CHECK(!!csr(cpu, CSR::Z) == (endRegVal == 0));
    auto new_reg = reg(cpu, target_reg);
    // Count the number of 1's in A[0:1].
    // If it is 0 or 2, then they agree in sign -> no signed overflow.
    // Otherwise, they disagree in sign -> signed overflow.
    auto top_2_bits = std::popcount(static_cast<uint16_t>(init_reg >> 14));
    CHECK(csr(cpu, CSR::V) == ((top_2_bits % 2) ? 1 : 0));
    // Carry out if high order bit was non-zero
    CHECK(csr(cpu, CSR::C) == ((init_reg & 0x8000) ? 1 : 0));
  }
}

template <typename Register, typename CSR, typename Mnemonic>
void inner_asr(PepISA3CPU::ISA isa, Register target_reg, Register other_reg, Mnemonic op) {
  auto [sys, mem, cpu] = make_cpu(isa);
  // Loop over a subset of possible values for the target register.
  u16 tmp;
  auto [init_reg] = GENERATE(table<u16>({0, 1, 0x7fff, 0x8000, 0x8FFF, 0xFFFF}));
  DYNAMIC_SECTION("with initial value " << init_reg) {
    auto endRegVal = static_cast<u16>(init_reg >> 1) | (init_reg & 0x8000 ? 1 << 15 : 0);

    // Object code for instruction under test.
    auto program = std::array<u8, 1>{(u8)op};

    cpu->registers()->clear(0);
    cpu->csrs()->clear(0);
    cpu->write_register(target_reg, init_reg);

    REQUIRE_NOTHROW(mem->write(0, {program.data(), program.size()}, rw));
    REQUIRE_NOTHROW(cpu->clock_tick(PulseSchedule::PulseIndex{0}, 0));

    CHECK(reg(cpu, Register::SP) == 0);
    CHECK(reg(cpu, other_reg) == 0);
    CHECK(reg(cpu, Register::PC) == 0x1);
    CHECK(reg(cpu, Register::IS) == (u8)op);
    // Check that target register had arithmetic performed.
    CHECK(reg(cpu, target_reg) == endRegVal);

    // Check that target register had arithmetic performed.
    CHECK(reg(cpu, target_reg) == endRegVal);
    // Check that target status bits match RTL.
    CHECK(csr(cpu, CSR::N) == (endRegVal & 0x8000 ? 1 : 0));
    CHECK(!!csr(cpu, CSR::Z) == (endRegVal == 0));
    auto new_reg = reg(cpu, target_reg);
    CHECK(csr(cpu, CSR::V) == 0);
    // Carry out if low order bit was non-zero
    CHECK(csr(cpu, CSR::C) == (init_reg & 0x1));
  }
}

} // namespace

TEST_CASE("(new) Pep/10, ASLA, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner_asl<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, Register::A, Register::X, MN::ASLA);
}
TEST_CASE("(new) Pep/10, ASLX, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner_asl<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, Register::X, Register::A, MN::ASLX);
}
TEST_CASE("(new) Pep/9, ASLA, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep9::Register;
  using CSR = isa::Pep9::CSR;
  using MN = isa::Pep9::Mnemonic;
  inner_asl<Register, CSR, MN>(PepISA3CPU::ISA::Pep9, Register::A, Register::X, MN::ASLA);
}
TEST_CASE("(new) Pep/9, ASLX, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep9::Register;
  using CSR = isa::Pep9::CSR;
  using MN = isa::Pep9::Mnemonic;
  inner_asl<Register, CSR, MN>(PepISA3CPU::ISA::Pep9, Register::X, Register::A, MN::ASLX);
}

TEST_CASE("(new) Pep/10, ASRA, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner_asr<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, Register::A, Register::X, MN::ASRA);
}
TEST_CASE("(new) Pep/10, ASRX, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner_asr<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, Register::X, Register::A, MN::ASRX);
}
TEST_CASE("(new) Pep/9, ASRA, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep9::Register;
  using CSR = isa::Pep9::CSR;
  using MN = isa::Pep9::Mnemonic;
  inner_asr<Register, CSR, MN>(PepISA3CPU::ISA::Pep9, Register::A, Register::X, MN::ASRA);
}
TEST_CASE("(new) Pep/9, ASRX, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep9::Register;
  using CSR = isa::Pep9::CSR;
  using MN = isa::Pep9::Mnemonic;
  inner_asr<Register, CSR, MN>(PepISA3CPU::ISA::Pep9, Register::X, Register::A, MN::ASRX);
}
