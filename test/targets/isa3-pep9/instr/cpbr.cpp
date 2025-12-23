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
#include "targets/pep9/isa3/cpu.hpp"
#include "utils/bits/swap.hpp"

namespace {
template <isa::Pep9::Register target_reg> void inner(isa::Pep9::Mnemonic op) {
  auto [mem, cpu] = make();
  // Loop over a subset of possible values for the target register.
  quint16 tmp;
  auto [init_reg] = GENERATE(table<quint16>({0, 1, 0x7fff, 0x8000, 0x8001, 0x8FFF, 0xFFFF}));
  for (uint16_t opspec : {0, 0x01, 0xFE, 0xFF, 0x80, 0x8000, 0x8001, 0xFFFE, 0xFFFF}) {
    auto endRegVal = static_cast<quint8>(init_reg + (~opspec + 1));

    // Object code for instruction under test.
    auto program = std::array<quint8, 3>{(quint8)op, static_cast<uint8_t>((opspec >> 8) & 0xff),
                                         static_cast<uint8_t>(opspec & 0xff)};

    cpu->regs()->clear(0);
    cpu->csrs()->clear(0);

    tmp = bits::hostOrder() != bits::Order::BigEndian ? bits::byteswap(init_reg) : init_reg;
    cpu->regs()->write(static_cast<quint16>(target_reg) * 2, {reinterpret_cast<quint8 *>(&tmp), 2}, rw);

    REQUIRE_NOTHROW(mem->write(0, {program.data(), program.size()}, rw));
    REQUIRE_NOTHROW(cpu->clock(0));

    tmp = bits::hostOrder() != bits::Order::BigEndian ? bits::byteswap(tmp) : tmp;
    CHECK(reg(cpu, isa::Pep9::Register::SP) == 0);
    CHECK(reg(cpu, isa::Pep9::Register::PC) == 0x3);
    CHECK(reg(cpu, isa::Pep9::Register::IS) == (quint8)op);
    // OS loaded the Mem[0x0001-0x0002].
    CHECK(reg(cpu, isa::Pep9::Register::OS) == opspec);
    // Check that target register did not change.
    CHECK(reg(cpu, target_reg) == init_reg);
    // Check that target status bits match RTL.
    CHECK(!!csr(cpu, isa::Pep9::CSR::Z) == (endRegVal == 0));
    CHECK(csr(cpu, isa::Pep9::CSR::V) == 0);
    CHECK(csr(cpu, isa::Pep9::CSR::C) == 0);
    CHECK(csr(cpu, isa::Pep9::CSR::N) == (endRegVal & 0x80 ? 1 : 0));
  }
}
} // namespace
TEST_CASE("Pep9::Mnemonic::CPBA", "[scope:targets][kind:int][target:pep9]") {
  using Register = isa::Pep9::Register;
  inner<Register::A>(isa::Pep9::Mnemonic::CPBA);
}
TEST_CASE("Pep9::Mnemonic::CPBX", "[scope:targets][kind:int][target:pep9]") {
  using Register = isa::Pep9::Register;
  inner<Register::X>(isa::Pep9::Mnemonic::CPBX);
}
