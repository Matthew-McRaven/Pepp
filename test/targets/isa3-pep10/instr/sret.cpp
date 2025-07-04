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
#include "./api.hpp"
#include "utils/bits/swap.hpp"
#include "sim/device/dense.hpp"
#include "targets/pep10/isa3/cpu.hpp"
#include "targets/isa3/helpers.hpp"

TEST_CASE("SRET", "[scope:targets][kind:int][target:pep10]") {
  auto op = isa::Pep10::Mnemonic ::SRET;
  auto [mem, cpu] = make();

  quint8 tmp8 = 0;
  quint16 tmp = 0;
  auto tmpSpan = bits::span<quint8>{reinterpret_cast<quint8 *>(tmp), sizeof(tmp)};
  const quint8 truth[] = {
      /*NZVC*/ 0b1101, /*A*/ 0x11, 0x22,        /*X*/ 0xBA, 0xAD,
      /*PC*/ 0xCA,     0xDE,       /*sp*/ 0xFE, 0xED,       (quint8)isa::Pep10::Mnemonic::SCALL};
  quint8 buf[sizeof(truth)];
  auto program = std::array<quint8, 1>{(quint8)isa::Pep10::Mnemonic::SRET};

  auto osSP = std::array<quint8, 2>{0x80, 0x86};

  cpu->regs()->clear(0);
  cpu->csrs()->clear(0);

  REQUIRE_NOTHROW(targets::isa::writeRegister<isa::Pep10>(cpu->regs(), isa::Pep10::Register::SP, 0x8086 - 12, rw));
  REQUIRE_NOTHROW(mem->write(0x0000, {program.data(), program.size()}, rw));
  REQUIRE_NOTHROW(mem->write(0x8086 - 12, {truth, sizeof(truth)}, rw));

  REQUIRE_NOTHROW(cpu->clock(0));

  REQUIRE_NOTHROW(targets::isa::readPackedCSR<isa::Pep10>(cpu->csrs(), tmp8, rw));
  CHECK(tmp8 == truth[0]);
  REQUIRE_NOTHROW(targets::isa::readRegister<isa::Pep10>(cpu->regs(), isa::Pep10::Register::A, tmp, rw));
  CHECK(tmp == (truth[1] << 8 | truth[2]));
  REQUIRE_NOTHROW(targets::isa::readRegister<isa::Pep10>(cpu->regs(), isa::Pep10::Register::X, tmp, rw));
  CHECK(tmp == (truth[3] << 8 | truth[4]));
  REQUIRE_NOTHROW(targets::isa::readRegister<isa::Pep10>(cpu->regs(), isa::Pep10::Register::PC, tmp, rw));
  CHECK(tmp == (truth[5] << 8 | truth[6]));
  REQUIRE_NOTHROW(targets::isa::readRegister<isa::Pep10>(cpu->regs(), isa::Pep10::Register::SP, tmp, rw));
  CHECK(tmp == (truth[7] << 8 | truth[8]));

  REQUIRE_NOTHROW(mem->read((quint16)isa::Pep10::MemoryVectors::SystemStackPtr,
                            {reinterpret_cast<quint8 *>(&tmp), sizeof(tmp)}, rw));

  tmp = bits::hostOrder() != bits::Order::BigEndian ? bits::byteswap(tmp) : tmp;
  CHECK(tmp == 0x8086);
}
