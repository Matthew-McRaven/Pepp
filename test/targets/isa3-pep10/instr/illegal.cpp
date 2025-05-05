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
#include "targets/pep10/isa3/cpu.hpp"
#include "targets/isa3/helpers.hpp"

namespace {
void inner(quint8 op) {
  auto [mem, cpu] = make();

  // Object code for instruction under test.
  auto program = std::array<quint8, 1>{(quint8)op};

  cpu->regs()->clear(0);
  cpu->csrs()->clear(0);

  REQUIRE_NOTHROW(mem->write(0, {program.data(), program.size()}, rw));
  REQUIRE_THROWS(cpu->clock(0));
  REQUIRE(cpu->status() == targets::pep10::isa::CPU::Status::IllegalOpcode);

  CHECK(reg(cpu, isa::Pep10::Register::SP) == 0);
  CHECK(reg(cpu, isa::Pep10::Register::PC) == 0x1);
  CHECK(reg(cpu, isa::Pep10::Register::IS) == (quint8)op);
  CHECK(csr(cpu, isa::Pep10::CSR::N) == 0);
  CHECK(csr(cpu, isa::Pep10::CSR::Z) == 0);
  CHECK(csr(cpu, isa::Pep10::CSR::V) == 0);
  CHECK(csr(cpu, isa::Pep10::CSR::C) == 0);
}
} // namespace

TEST_CASE("Illegal Opcodes", "[scope:targets][kind:int][target:pep10]") {
  using namespace Qt::StringLiterals;
  SECTION("Opcode 0") { inner(0); }
  for (quint8 opcode = 8; opcode < 0x18; opcode++) {
    DYNAMIC_SECTION(u"Opcode %1"_s.arg(opcode).toStdString()) { inner(opcode); }
  }
}
