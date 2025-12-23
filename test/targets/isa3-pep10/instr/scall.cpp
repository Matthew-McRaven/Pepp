/*
 * Copyright (c) 2024 J. Stanley Warford, Matthew McRaven
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

TEST_CASE("SCALL", "[scope:targets][kind:int][target:pep10]") {
  auto op = isa::Pep10::Mnemonic ::SCALL;
  auto [mem, cpu] = make();

  quint16 tmp = 0;
  auto tmpSpan = bits::span<quint8>{reinterpret_cast<quint8 *>(tmp), sizeof(tmp)};
  const quint8 truth[] = {/*NZVC*/ 0b1101,
                          /*A*/ 0x11,      0x22,
                          /*X*/ 0xBA,      0xAD,
                          /*PC*/ 0x00,     0x03,
                          /*sp*/ 0xFE,     0xED, (quint8)isa::Pep10::Mnemonic::SCALL, 0xCA, 0xBE};
  quint8 buf[sizeof(truth)];

  auto program = std::array<quint8, 3>{(quint8)isa::Pep10::Mnemonic::SCALL, 0xCA, 0xBE};
  auto osSP = std::array<quint8, 2>{0x80, 0x86};

  cpu->regs()->clear(0);
  cpu->csrs()->clear(0);

  REQUIRE_NOTHROW(targets::isa::writePackedCSR<isa::Pep10>(cpu->csrs(), truth[0], rw));
  tmp = bits::hostOrder() != bits::Order::BigEndian ? bits::byteswap(*(quint16 *)(truth + 1)) : *(quint16 *)(truth + 1);
  REQUIRE_NOTHROW(targets::isa::writeRegister<isa::Pep10>(cpu->regs(), isa::Pep10::Register::A, tmp, rw));
  tmp = bits::hostOrder() != bits::Order::BigEndian ? bits::byteswap(*(quint16 *)(truth + 3)) : *(quint16 *)(truth + 3);
  REQUIRE_NOTHROW(targets::isa::writeRegister<isa::Pep10>(cpu->regs(), isa::Pep10::Register::X, tmp, rw));
  tmp = bits::hostOrder() != bits::Order::BigEndian ? bits::byteswap(*(quint16 *)(truth + 7)) : *(quint16 *)(truth + 7);
  REQUIRE_NOTHROW(targets::isa::writeRegister<isa::Pep10>(cpu->regs(), isa::Pep10::Register::SP, tmp, rw));
  REQUIRE_NOTHROW(mem->write(0x0000, {program.data(), program.size()}, rw));
  REQUIRE_NOTHROW(mem->write((quint16)isa::Pep10::MemoryVectors::SystemStackPtr, {osSP.data(), osSP.size()}, rw));

  REQUIRE_NOTHROW(cpu->clock(0));

  REQUIRE_NOTHROW(targets::isa::readRegister<isa::Pep10>(cpu->regs(), isa::Pep10::Register::SP, tmp, rw));
  CHECK(tmp + 12 == 0x8086);
  REQUIRE_NOTHROW(mem->read(tmp, {buf, sizeof(buf)}, rw));

  for (int it = 0; it < sizeof(truth); it++)
    CHECK(buf[it] == truth[it]);
}
