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

#include "sim3/subsystems/ram/dense.hpp"
#include "./api.hpp"
#include "targets/isa3/helpers.hpp"
#include "targets/pep10/isa3/cpu.hpp"
#include "utils/bits/swap.hpp"

namespace {
template <isa::Pep10::Register target_reg, isa::Pep10::Register other_reg> void inner(isa::Pep10::Mnemonic op) {
  auto [mem, cpu] = make();
  // Loop over a subset of possible values for the target register.
  quint16 tmp;
  auto [init_reg] = GENERATE(table<quint16>({0, 1, 0x7fff, 0x8000, 0x8FFF, 0xFFFF}));
  DYNAMIC_SECTION("with initial value " << init_reg) {
    for (uint16_t opspec = 0; static_cast<uint32_t>(opspec) + 1 < 0x1'0000; opspec++) {
      auto endRegVal = static_cast<quint16>((~opspec + 1) + init_reg);

      // Object code for instruction under test.
      auto program = std::array<quint8, 3>{(quint8)op, static_cast<uint8_t>((opspec >> 8) & 0xff),
                                           static_cast<uint8_t>(opspec & 0xff)};

      cpu->regs()->clear(0);
      cpu->csrs()->clear(0);
      tmp = bits::hostOrder() != bits::Order::BigEndian ? bits::byteswap(init_reg) : init_reg;
      cpu->regs()->write(static_cast<quint16>(target_reg) * 2, {reinterpret_cast<quint8 *>(&tmp), 2}, rw);

      REQUIRE_NOTHROW(mem->write(0, {program.data(), program.size()}, rw));
      REQUIRE_NOTHROW(cpu->clock(0));

      CHECK(reg(cpu, isa::Pep10::Register::SP) == 0);
      CHECK(reg(cpu, other_reg) == 0);
      CHECK(reg(cpu, isa::Pep10::Register::PC) == 0x3);
      CHECK(reg(cpu, isa::Pep10::Register::IS) == (quint8)op);
      // OS loaded the Mem[0x0001-0x0002].
      CHECK(reg(cpu, isa::Pep10::Register::OS) == opspec);
      // Check that target register had arithmetic performed.
      CHECK(reg(cpu, target_reg) == endRegVal);
      // Check that target status bits match RTL.
      CHECK(csr(cpu, isa::Pep10::CSR::N) == (endRegVal & 0x8000 ? 1 : 0));
      CHECK(!!csr(cpu, isa::Pep10::CSR::Z) == (endRegVal == 0));
      auto input_sign_match = ~(init_reg ^ (~opspec + 1)) & 0x8000;
      auto output_sign_match = ~(endRegVal ^ init_reg) & 0x8000;
      // Explicitly check that if input signs do no match, thatV is always
      // false.
      bool signed_overflow = input_sign_match ? input_sign_match != output_sign_match : false;
      CHECK(!!csr(cpu, isa::Pep10::CSR::V) == signed_overflow);
      // Don't use bit twiddling here. This validates that my bit twiddles in
      // the CPU are logically equivalent to to carrying into bit 17 of a
      // 32-bit type.
      auto result = static_cast<uint32_t>(init_reg) + static_cast<uint16_t>(~opspec + 1);
      CHECK(csr(cpu, isa::Pep10::CSR::C) == (result >= 0x1'0000 ? 1 : 0));
    }
  }
}
} // namespace

TEST_CASE("SUBA, i", "[scope:targets][kind:int][target:pep10]") {
  using Register = isa::Pep10::Register;
  inner<Register::A, Register::X>(isa::Pep10::Mnemonic::SUBA);
}
TEST_CASE("SUBX, i", "[scope:targets][kind:int][target:pep10]") {
  using Register = isa::Pep10::Register;
  inner<Register::X, Register::A>(isa::Pep10::Mnemonic::SUBX);
}
