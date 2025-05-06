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
#include "utils/bits/swap.hpp"
#include "sim/device/dense.hpp"
#include "targets/pep9/isa3/cpu.hpp"
#include "targets/isa3/helpers.hpp"

namespace {
void inner(isa::Pep9::Mnemonic op) {
  auto [mem, cpu] = make();
  quint8 buf[2];
  auto bufSpan = bits::span<quint8>{buf};
  for (quint16 opspec = 0; static_cast<quint32>(opspec) + 1 < quint32(0x1'0000); opspec++) {
    // Object code for instruction under test.
    auto program = std::array<quint8, 3>{(quint8)op, static_cast<uint8_t>((opspec >> 8) & 0xff),
                                         static_cast<uint8_t>(opspec & 0xff)};

    cpu->regs()->clear(0);
    cpu->csrs()->clear(0);
    constexpr quint8 truth[2] = {0x11, 0x25};
    targets::isa::writeRegister<isa::Pep9>(cpu->regs(), isa::Pep9::Register::SP, 0xFFFF, rw);
    targets::isa::writeRegister<isa::Pep9>(cpu->regs(), isa::Pep9::Register::PC, 0x1122, rw);

    REQUIRE_NOTHROW(mem->write(0x1122, {program.data(), program.size()}, rw));
    REQUIRE_NOTHROW(cpu->clock(0));

    CHECK(reg(cpu, isa::Pep9::Register::SP) == 0xFFFD);
    CHECK(reg(cpu, isa::Pep9::Register::PC) == opspec);
    CHECK(reg(cpu, isa::Pep9::Register::IS) == (quint8)op);
    // OS loaded the Mem[0x0001-0x0002].
    CHECK(reg(cpu, isa::Pep9::Register::OS) == opspec);
    REQUIRE_NOTHROW(mem->read(0xFFFD, bufSpan, rw));
    for (int it = 0; it < 2; it++)
      CHECK(buf[it] == truth[it]);
  }
}
} // namespace
TEST_CASE("Pep9::Mnemonic::CALL", "[scope:targets][kind:int][target:pep9]") {
  using Register = isa::Pep9::Register;
  inner(isa::Pep9::Mnemonic::CALL);
}
