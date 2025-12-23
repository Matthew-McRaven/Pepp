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
void inner(isa::Pep10::Mnemonic op) {
  using Register = isa::Pep10::Register;
  auto [mem, cpu] = make();
  quint16 tmp;
  for (quint16 flg = 0; flg < 0b1'00'00; flg++) {
    // Object code for instruction under test.
    auto program = std::array<quint8, 1>{(quint8)op};

    cpu->regs()->clear(0);
    cpu->csrs()->clear(0);

    if (op == isa::Pep10::Mnemonic::MOVAFLG) {
      targets::isa::writeRegister<isa::Pep10>(cpu->regs(), Register::A, flg, rw);
    } else {
      targets::isa::writePackedCSR<isa::Pep10>(cpu->csrs(), flg, rw);
    }

    REQUIRE_NOTHROW(mem->write(0, {program.data(), program.size()}, rw));
    REQUIRE_NOTHROW(cpu->clock(0));

    CHECK(reg(cpu, isa::Pep10::Register::PC) == 0x1);
    CHECK(reg(cpu, isa::Pep10::Register::IS) == (quint8)op);
    CHECK(reg(cpu, isa::Pep10::Register::OS) == 0);
    CHECK(reg(cpu, isa::Pep10::Register::X) == 0);
    // Check that target register had arithmetic performed.
    CHECK(reg(cpu, Register::A) == flg);
    CHECK(csr(cpu, isa::Pep10::CSR::N) == (flg & 0b1000 ? 1 : 0));
    CHECK(csr(cpu, isa::Pep10::CSR::Z) == (flg & 0b0100 ? 1 : 0));
    CHECK(csr(cpu, isa::Pep10::CSR::V) == (flg & 0b0010 ? 1 : 0));
    CHECK(csr(cpu, isa::Pep10::CSR::C) == (flg & 0b0001 ? 1 : 0));
  }
}
} // namespace

TEST_CASE("MOVAFLG", "[scope:targets][kind:int][target:pep10]") { inner(isa::Pep10::Mnemonic::MOVAFLG); }
TEST_CASE("MOVFLGA", "[scope:targets][kind:int][target:pep10]") { inner(isa::Pep10::Mnemonic::MOVFLGA); }
