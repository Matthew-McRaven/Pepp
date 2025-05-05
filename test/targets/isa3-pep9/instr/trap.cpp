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
#include "targets/pep9/isa3/cpu.hpp"
#include "targets/isa3/helpers.hpp"

void smoke_unary(isa::Pep9::Mnemonic op) {
  auto [mem, cpu] = make();

  quint16 tmp = 0;
  auto tmpSpan = bits::span<quint8>{reinterpret_cast<quint8 *>(tmp), sizeof(tmp)};
  const quint8 truth[] = {/*NZVC*/ 0b1101, /*A*/ 0x11, 0x22,        /*X*/ 0xBA, 0xAD,
                          /*PC*/ 0x00,     0x01,       /*sp*/ 0xFE, 0xED,       (quint8)op};
  quint8 buf[sizeof(truth)];

  auto program = std::array<quint8, 1>{(quint8)op};
  auto osSP = std::array<quint8, 2>{0x80, 0x86};

  cpu->regs()->clear(0);
  cpu->csrs()->clear(0);

  REQUIRE_NOTHROW(targets::isa::writePackedCSR<isa::Pep9>(cpu->csrs(), truth[0], rw));
  tmp = bits::hostOrder() != bits::Order::BigEndian ? bits::byteswap(*(quint16 *)(truth + 1)) : *(quint16 *)(truth + 1);
  REQUIRE_NOTHROW(targets::isa::writeRegister<isa::Pep9>(cpu->regs(), isa::Pep9::Register::A, tmp, rw));
  tmp = bits::hostOrder() != bits::Order::BigEndian ? bits::byteswap(*(quint16 *)(truth + 3)) : *(quint16 *)(truth + 3);
  REQUIRE_NOTHROW(targets::isa::writeRegister<isa::Pep9>(cpu->regs(), isa::Pep9::Register::X, tmp, rw));
  tmp = bits::hostOrder() != bits::Order::BigEndian ? bits::byteswap(*(quint16 *)(truth + 7)) : *(quint16 *)(truth + 7);
  REQUIRE_NOTHROW(targets::isa::writeRegister<isa::Pep9>(cpu->regs(), isa::Pep9::Register::SP, tmp, rw));
  REQUIRE_NOTHROW(mem->write(0x0000, {program.data(), program.size()}, rw));
  REQUIRE_NOTHROW(mem->write((quint16)isa::Pep9::MemoryVectors::SystemStackPtr, {osSP.data(), osSP.size()}, rw));

  REQUIRE_NOTHROW(cpu->clock(0));

  REQUIRE_NOTHROW(targets::isa::readRegister<isa::Pep9>(cpu->regs(), isa::Pep9::Register::SP, tmp, rw));
  CHECK(tmp + 10 == 0x8086);
  REQUIRE_NOTHROW(mem->read(tmp, {buf, sizeof(buf)}, rw));

  for (int it = 0; it < sizeof(truth); it++) CHECK(buf[it] == truth[it]);
}

void smoke_nonunary(isa::Pep9::Mnemonic op) {
  auto [mem, cpu] = make();

  quint16 tmp = 0;
  auto tmpSpan = bits::span<quint8>{reinterpret_cast<quint8 *>(tmp), sizeof(tmp)};
  const quint8 truth[] = {/*NZVC*/ 0b1101, /*A*/ 0x11, 0x22,        /*X*/ 0xBA, 0xAD,
                          /*PC*/ 0x00,     0x03,       /*sp*/ 0xFE, 0xED,       (quint8)op};
  quint8 buf[sizeof(truth)];

  auto program = std::array<quint8, 3>{(quint8)op, 0xCA, 0xBE};
  auto osSP = std::array<quint8, 2>{0x80, 0x86};

  cpu->regs()->clear(0);
  cpu->csrs()->clear(0);

  REQUIRE_NOTHROW(targets::isa::writePackedCSR<isa::Pep9>(cpu->csrs(), truth[0], rw));
  tmp = bits::hostOrder() != bits::Order::BigEndian ? bits::byteswap(*(quint16 *)(truth + 1)) : *(quint16 *)(truth + 1);
  REQUIRE_NOTHROW(targets::isa::writeRegister<isa::Pep9>(cpu->regs(), isa::Pep9::Register::A, tmp, rw));
  tmp = bits::hostOrder() != bits::Order::BigEndian ? bits::byteswap(*(quint16 *)(truth + 3)) : *(quint16 *)(truth + 3);
  REQUIRE_NOTHROW(targets::isa::writeRegister<isa::Pep9>(cpu->regs(), isa::Pep9::Register::X, tmp, rw));
  tmp = bits::hostOrder() != bits::Order::BigEndian ? bits::byteswap(*(quint16 *)(truth + 7)) : *(quint16 *)(truth + 7);
  REQUIRE_NOTHROW(targets::isa::writeRegister<isa::Pep9>(cpu->regs(), isa::Pep9::Register::SP, tmp, rw));
  REQUIRE_NOTHROW(mem->write(0x0000, {program.data(), program.size()}, rw));
  REQUIRE_NOTHROW(mem->write((quint16)isa::Pep9::MemoryVectors::SystemStackPtr, {osSP.data(), osSP.size()}, rw));

  REQUIRE_NOTHROW(cpu->clock(0));

  REQUIRE_NOTHROW(targets::isa::readRegister<isa::Pep9>(cpu->regs(), isa::Pep9::Register::SP, tmp, rw));
  CHECK(tmp + 10 == 0x8086);
  REQUIRE_NOTHROW(mem->read(tmp, {buf, sizeof(buf)}, rw));

  for (int it = 0; it < sizeof(truth); it++) CHECK(buf[it] == truth[it]);
}

TEST_CASE("Pep9::Mnemonic::NOP0", "[scope:targets][kind:int][target:pep9]") { smoke_unary(isa::Pep9::Mnemonic::NOP0); }
TEST_CASE("Pep9::Mnemonic::NOP1", "[scope:targets][kind:int][target:pep9]") { smoke_unary(isa::Pep9::Mnemonic::NOP1); }
TEST_CASE("Pep9::Mnemonic::NOP", "[scope:targets][kind:int][target:pep9]") { smoke_nonunary(isa::Pep9::Mnemonic::NOP); }
TEST_CASE("Pep9::Mnemonic::DECI", "[scope:targets][kind:int][target:pep9]") {
  smoke_nonunary(isa::Pep9::Mnemonic::DECI);
}
TEST_CASE("Pep9::Mnemonic::DECO", "[scope:targets][kind:int][target:pep9]") {
  smoke_nonunary(isa::Pep9::Mnemonic::DECO);
}
TEST_CASE("Pep9::Mnemonic::HEXO", "[scope:targets][kind:int][target:pep9]") {
  smoke_nonunary(isa::Pep9::Mnemonic::HEXO);
}
TEST_CASE("Pep9::Mnemonic::STRO", "[scope:targets][kind:int][target:pep9]") {
  smoke_nonunary(isa::Pep9::Mnemonic::STRO);
}
