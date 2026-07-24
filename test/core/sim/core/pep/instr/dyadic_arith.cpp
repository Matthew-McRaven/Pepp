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
void inner_add(PepISA3CPU::ISA isa, Register target_reg, Register other_reg, Mnemonic op) {
  auto [sys, mem, cpu] = make_cpu(isa);
  // Loop over a subset of possible values for the target register.
  u16 tmp;
  auto [init_reg] = GENERATE(table<u16>({0, 1, 0x7fff, 0x8000, 0x8FFF, 0xFFFF}));
  DYNAMIC_SECTION("with initial value " << init_reg) {
    for (u16 opspec = 0; static_cast<u32>(opspec) + 1 < 0x1'0000; opspec++) {
      auto endRegVal = static_cast<u16>(opspec + init_reg);

      // Object code for instruction under test.
      auto program =
          std::array<u8, 3>{(u8)op, static_cast<uint8_t>((opspec >> 8) & 0xff), static_cast<uint8_t>(opspec & 0xff)};

      cpu->registers()->clear(0);
      cpu->csrs()->clear(0);
      cpu->write_register(target_reg, init_reg);

      REQUIRE_NOTHROW(mem->write(0, {program.data(), program.size()}, rw));
      REQUIRE_NOTHROW(cpu->clock_tick(PulseSchedule::PulseIndex{0}, 0));

      CHECK(reg(cpu, Register::SP) == 0);
      CHECK(reg(cpu, other_reg) == 0);
      CHECK(reg(cpu, Register::PC) == 0x3);
      CHECK(reg(cpu, Register::IS) == (u8)op);
      // OS loaded the Mem[0x0001-0x0002].
      CHECK(reg(cpu, Register::OS) == opspec);
      // Check that target register had arithmetic performed.
      CHECK(reg(cpu, target_reg) == endRegVal);
      // Check that target status bits match RTL.
      CHECK(csr(cpu, CSR::N) == (endRegVal & 0x8000 ? 1 : 0));
      CHECK(!!csr(cpu, CSR::Z) == (endRegVal == 0));
      auto input_sign_match = ~(init_reg ^ opspec) & 0x8000;
      auto output_sign_match = ~(endRegVal ^ init_reg) & 0x8000;
      // Explicitly check that if input signs do no match, thatV is always
      // false.
      auto signed_overflow = input_sign_match ? input_sign_match != output_sign_match : false;
      CHECK(!!csr(cpu, CSR::V) == signed_overflow);
      // Don't use bit twiddling here. This validates that my bit twiddles in
      // the CPU are logically equivalent to to carrying into bit 17 of a
      // 32-bit type.
      u32 result = static_cast<u32>(init_reg) + static_cast<u32>(opspec);
      CHECK(csr(cpu, CSR::C) == (result >= 0x1'0000 ? 1 : 0));
    }
  }
}
template <typename Register, typename CSR, typename Mnemonic>
void inner_sub(PepISA3CPU::ISA isa, Register target_reg, Register other_reg, Mnemonic op) {
  auto [sys, mem, cpu] = make_cpu(isa);
  // Loop over a subset of possible values for the target register.
  u16 tmp;
  auto [init_reg] = GENERATE(table<u16>({0, 1, 0x7fff, 0x8000, 0x8FFF, 0xFFFF}));
  DYNAMIC_SECTION("with initial value " << init_reg) {
    for (u16 opspec = 0; static_cast<u32>(opspec) + 1 < 0x1'0000; opspec++) {
      auto endRegVal = static_cast<u16>((~opspec + 1) + init_reg);

      // Object code for instruction under test.
      auto program = std::array<u8, 3>{(u8)op, static_cast<u8>((opspec >> 8) & 0xff), static_cast<u8>(opspec & 0xff)};

      cpu->registers()->clear(0);
      cpu->csrs()->clear(0);
      cpu->write_register(target_reg, init_reg);

      REQUIRE_NOTHROW(mem->write(0, {program.data(), program.size()}, rw));
      REQUIRE_NOTHROW(cpu->clock_tick(PulseSchedule::PulseIndex{0}, 0));

      CHECK(reg(cpu, Register::SP) == 0);
      CHECK(reg(cpu, other_reg) == 0);
      CHECK(reg(cpu, Register::PC) == 0x3);
      CHECK(reg(cpu, Register::IS) == (u8)op);
      // OS loaded the Mem[0x0001-0x0002].
      CHECK(reg(cpu, Register::OS) == opspec);
      // Check that target register had arithmetic performed.
      CHECK(reg(cpu, target_reg) == endRegVal);
      // Check that target status bits match RTL.
      CHECK(csr(cpu, CSR::N) == (endRegVal & 0x8000 ? 1 : 0));
      CHECK(!!csr(cpu, CSR::Z) == (endRegVal == 0));
      auto input_sign_match = ~(init_reg ^ (~opspec + 1)) & 0x8000;
      auto output_sign_match = ~(endRegVal ^ init_reg) & 0x8000;
      // Explicitly check that if input signs do no match, thatV is always
      // false.
      bool signed_overflow = input_sign_match ? input_sign_match != output_sign_match : false;
      CHECK(!!csr(cpu, CSR::V) == signed_overflow);
      // Don't use bit twiddling here. This validates that my bit twiddles in
      // the CPU are logically equivalent to to carrying into bit 17 of a
      // 32-bit type.
      auto result = static_cast<uint32_t>(init_reg) + static_cast<uint16_t>(~opspec + 1);
      CHECK(csr(cpu, isa::Pep10::CSR::C) == (result >= 0x1'0000 ? 1 : 0));
    }
  }
}
} // namespace

TEST_CASE("(new) Pep/10, ADDA, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner_add<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, Register::A, Register::X, MN::ADDA);
}
TEST_CASE("(new) Pep/10, ADDX, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner_add<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, Register::X, Register::A, MN::ADDX);
}
TEST_CASE("(new) Pep/9, ADDA, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep9::CSR;
  using MN = isa::Pep9::Mnemonic;
  // inner_add<Register, CSR, MN>(PepISA3CPU::ISA::Pep9, Register::A, Register::X, MN::ADDA);
}
TEST_CASE("(new) Pep/9,, ADDX, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep9::Register;
  using CSR = isa::Pep9::CSR;
  using MN = isa::Pep9::Mnemonic;
  // inner_add<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, Register::X, Register::A, MN::ADDX);
}

TEST_CASE("(new) Pep/10, SUBA, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner_sub<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, Register::A, Register::X, MN::SUBA);
}
TEST_CASE("(new) Pep/10, SUBX, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner_sub<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, Register::X, Register::A, MN::SUBX);
}