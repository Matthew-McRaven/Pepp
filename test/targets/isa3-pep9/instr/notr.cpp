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

#include "./api.hpp"
#include "bits/operations/swap.hpp"
#include "sim/device/dense.hpp"
#include "targets/pep9/isa3/cpu.hpp"
#include "targets/isa3/helpers.hpp"

namespace {
template <isa::Pep9::Register target_reg> void inner_notr(isa::Pep9::Mnemonic op) {
  auto [mem, cpu] = make();
  quint16 tmp;
  auto [init_reg] = GENERATE(table<quint16>({0, 1, 0x7fff, 0x8000, 0x8FFF, 0xFFFF}));
  DYNAMIC_SECTION("with initial value " << init_reg) {
    auto endRegVal = static_cast<quint16>(~init_reg);

    // Object code for instruction under test.
    auto program = std::array<quint8, 1>{(quint8)op};

    cpu->regs()->clear(0);
    cpu->csrs()->clear(0);
    tmp = bits::hostOrder() != bits::Order::BigEndian ? bits::byteswap(init_reg) : init_reg;
    cpu->regs()->write(static_cast<quint16>(target_reg) * 2, {reinterpret_cast<quint8 *>(&tmp), 2}, rw);

    REQUIRE_NOTHROW(mem->write(0, {program.data(), program.size()}, rw));
    REQUIRE_NOTHROW(cpu->clock(0));

    CHECK(reg(cpu, isa::Pep9::Register::SP) == 0);
    CHECK(reg(cpu, isa::Pep9::Register::PC) == 0x1);
    CHECK(reg(cpu, isa::Pep9::Register::IS) == (quint8)op);
    CHECK(reg(cpu, isa::Pep9::Register::OS) == 0);
    // Check that target register had arithmetic performed.
    CHECK(reg(cpu, target_reg) == endRegVal);
    // Check that target status bits match RTL.
    CHECK(csr(cpu, isa::Pep9::CSR::N) == (endRegVal & 0x8000 ? 1 : 0));
    CHECK(!!csr(cpu, isa::Pep9::CSR::Z) == (endRegVal == 0));
  }
}
} // namespace
TEST_CASE("Pep9::Mnemonic::NOTA", "[scope:targets][kind:int][target:pep9]") {
  using Register = isa::Pep9::Register;
  inner_notr<Register::A>(isa::Pep9::Mnemonic::NOTA);
}
TEST_CASE("Pep9::Mnemonic::NOTX", "[scope:targets][kind:int][target:pep9]") {
  using Register = isa::Pep9::Register;
  inner_notr<Register::X>(isa::Pep9::Mnemonic::NOTX);
}
