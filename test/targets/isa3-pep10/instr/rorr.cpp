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

#include "sim3/cores/pep/traced_helpers.hpp"
#include "./api.hpp"
#include "sim3/subsystems/ram/dense.hpp"
#include "sim3/cores/pep/traced_pep10_isa3.hpp"
#include "utils/bits/swap.hpp"

namespace {
template <isa::Pep10::Register target_reg> void inner(isa::Pep10::Mnemonic op) {
  auto [mem, cpu] = make();
  quint16 tmp;
  auto [init_reg] = GENERATE(table<quint16>({0, 1, 0x7fff, 0x8000, 0x8FFF, 0xFFFF}));
  auto [carry] = GENERATE(table<quint8>({0, 1}));
  DYNAMIC_SECTION("with initial value==" << init_reg << " and carry==" << carry) {
    auto endRegVal = static_cast<quint16>(static_cast<quint16>(init_reg >> 1) | (carry ? 1 << 15 : 0));

    // Object code for instruction under test.
    auto program = std::array<quint8, 1>{(quint8)op};

    cpu->regs()->clear(0);
    cpu->csrs()->clear(0);
    tmp = bits::hostOrder() != bits::Order::BigEndian ? bits::byteswap(init_reg) : init_reg;
    cpu->regs()->write(static_cast<quint16>(target_reg) * 2, {reinterpret_cast<quint8 *>(&tmp), 2}, rw);
    cpu->csrs()->write(static_cast<quint8>(::isa::Pep10::CSR::C), {&carry, 1}, rw);

    REQUIRE_NOTHROW(mem->write(0, {program.data(), program.size()}, rw));
    REQUIRE_NOTHROW(cpu->clock(0));

    CHECK(reg(cpu, isa::Pep10::Register::SP) == 0);
    CHECK(reg(cpu, isa::Pep10::Register::PC) == 0x1);
    CHECK(reg(cpu, isa::Pep10::Register::IS) == (quint8)op);
    CHECK(reg(cpu, isa::Pep10::Register::OS) == 0);
    // Check that target register had arithmetic performed.
    CHECK(reg(cpu, target_reg) == endRegVal);
    // Check that target status bits match RTL.
    CHECK(csr(cpu, isa::Pep10::CSR::N) == (endRegVal & 0x8000 ? 1 : 0));
    CHECK(csr(cpu, isa::Pep10::CSR::Z) == (endRegVal == 0 ? 1 : 0));
    auto new_reg = reg(cpu, target_reg);
    CHECK(csr(cpu, isa::Pep10::CSR::V) == 0);
    // Carry out if low order bit was non-zero
    CHECK(csr(cpu, isa::Pep10::CSR::C) == quint16(init_reg & 0x1));
  }
}
} // namespace

TEST_CASE("RORA", "[scope:targets][kind:int][target:pep10]") {
  using Register = isa::Pep10::Register;
  inner<Register::A>(isa::Pep10::Mnemonic::RORA);
}
TEST_CASE("RORX", "[scope:targets][kind:int][target:pep10]") {
  using Register = isa::Pep10::Register;
  inner<Register::X>(isa::Pep10::Mnemonic::RORX);
}
