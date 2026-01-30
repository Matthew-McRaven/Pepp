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
#include "core/libs/bitmanip/swap.hpp"
#include "sim3/cores/pep/traced_helpers.hpp"
#include "sim3/cores/pep/traced_pep9_isa3.hpp"
#include "sim3/subsystems/ram/dense.hpp"

namespace {
template <isa::Pep9::Register target_reg> void inner(isa::Pep9::Mnemonic op, quint8 length) {
  auto [mem, cpu] = make();
  // Loop over a subset of possible values for the target register.
  auto [init_reg] = GENERATE(table<quint16>({0, 1, 0x7fff, 0x8000, 0x8FFF, 0xFFFF}));
  quint16 tmp;
  auto bufSpan = bits::span<quint8>{(quint8 *)&tmp, length};
  // Simple memory used in this test won't wrap around.
  for (uint16_t opspec : {0, 0x01, 0xFE, 0xFF, 0x80, 0x8000, 0x8001, 0xFFFE}) {
    auto endRegVal = opspec & 0xff;

    // Object code for instruction under test.
    auto program =
        std::array<quint8, 3>{static_cast<quint8>(1 + (quint8)op), // offset by 1 to get direct addressing.
                              static_cast<uint8_t>((opspec >> 8) & 0xff), static_cast<uint8_t>(opspec & 0xff)};

    cpu->regs()->clear(0);
    cpu->csrs()->clear(0);
    tmp = bits::hostOrder() != bits::Order::BigEndian ? bits::byteswap(init_reg) : init_reg;
    cpu->regs()->write(static_cast<quint16>(target_reg) * 2, {reinterpret_cast<quint8 *>(&tmp), 2}, rw);

    REQUIRE_NOTHROW(mem->write(0, {program.data(), program.size()}, rw));
    REQUIRE_NOTHROW(cpu->clock(0));

    CHECK(reg(cpu, isa::Pep9::Register::SP) == 0);
    CHECK(reg(cpu, isa::Pep9::Register::PC) == 0x3);
    CHECK(reg(cpu, isa::Pep9::Register::IS) == 1 + (quint8)op);
    // OS loaded the Mem[0x0001-0x0002].
    CHECK(reg(cpu, isa::Pep9::Register::OS) == opspec);
    REQUIRE_NOTHROW(mem->read(opspec, bufSpan, rw));
    // May need to byte-swap on read from main-memory.
    tmp = bits::hostOrder() != bits::Order::BigEndian ? bits::byteswap(tmp) : tmp;
    if (length == 1)
      CHECK(bufSpan[0] == (init_reg & 0xff));
    else
      CHECK(tmp == init_reg);
  }
}
} // namespace

TEST_CASE("Pep9::Mnemonic::STBA", "[scope:targets][kind:int][target:pep9]") {
  using Register = isa::Pep9::Register;
  inner<Register::A>(isa::Pep9::Mnemonic::STBA, 1);
}
TEST_CASE("Pep9::Mnemonic::STBX", "[scope:targets][kind:int][target:pep9]") {
  using Register = isa::Pep9::Register;
  inner<Register::X>(isa::Pep9::Mnemonic::STBX, 1);
}
TEST_CASE("Pep9::Mnemonic::STWA", "[scope:targets][kind:int][target:pep9]") {
  using Register = isa::Pep9::Register;
  inner<Register::A>(isa::Pep9::Mnemonic::STWA, 2);
}
TEST_CASE("Pep9::Mnemonic::STWX", "[scope:targets][kind:int][target:pep9]") {
  using Register = isa::Pep9::Register;
  inner<Register::X>(isa::Pep9::Mnemonic::STWX, 2);
}
