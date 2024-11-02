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
#include "targets/pep9/isa3/helpers.hpp"

namespace {
typedef bool (*should_branch)(bool n, bool z, bool v, bool c);

bool br_unconditional(bool n, bool z, bool v, bool c) { return true; };
bool br_le(bool n, bool z, bool v, bool c) { return n || z; };
bool br_lt(bool n, bool z, bool v, bool c) { return n; };
bool br_eq(bool n, bool z, bool v, bool c) { return z; };
bool br_ne(bool n, bool z, bool v, bool c) { return !z; };
bool br_ge(bool n, bool z, bool v, bool c) { return !n; };
bool br_gt(bool n, bool z, bool v, bool c) { return (!n) && (!z); };
bool br_v(bool n, bool z, bool v, bool c) { return v; };
bool br_c(bool n, bool z, bool v, bool c) { return c; };

void inner(isa::Pep9::Mnemonic op, should_branch taken) {
  auto [mem, cpu] = make();
  quint16 opspec = 0xFEED;
  for (quint8 nzvc = 0; nzvc < 0b1'00'00; nzvc++) {
    // Object code for instruction under test.
    auto program = std::array<quint8, 3>{(quint8)op, static_cast<uint8_t>((opspec >> 8) & 0xff),
                                         static_cast<uint8_t>(opspec & 0xff)};

    cpu->regs()->clear(0);
    cpu->csrs()->clear(0);
    quint8 tmp = 0;
    targets::pep9::isa::writePackedCSR(cpu->csrs(), nzvc, rw);
    targets::pep9::isa::readPackedCSR(cpu->csrs(), tmp, rw);
    auto [n, z, v, c] = targets::pep9::isa::unpackCSR(tmp);

    REQUIRE_NOTHROW(mem->write(0, {program.data(), program.size()}, rw));
    REQUIRE_NOTHROW(cpu->clock(0));

    CHECK(reg(cpu, isa::Pep9::Register::SP) == 0);
    CHECK(reg(cpu, isa::Pep9::Register::PC) == (taken(n, z, v, c) ? opspec : 0x03));
    CHECK(reg(cpu, isa::Pep9::Register::IS) == (quint8)op);
    // OS loaded the Mem[0x0001-0x0002].
    CHECK(reg(cpu, isa::Pep9::Register::OS) == opspec);
  }
}
} // namespace

TEST_CASE("Pep9::Mnemonic::BR", "[scope:targets][kind:int][target:pep9]") {
  using Register = isa::Pep9::Register;
  inner(isa::Pep9::Mnemonic::BR, br_unconditional);
}
TEST_CASE("Pep9::Mnemonic::BRLE", "[scope:targets][kind:int][target:pep9]") {
  using Register = isa::Pep9::Register;
  inner(isa::Pep9::Mnemonic::BRLE, br_le);
}
TEST_CASE("Pep9::Mnemonic::BRLT", "[scope:targets][kind:int][target:pep9]") {
  using Register = isa::Pep9::Register;
  inner(isa::Pep9::Mnemonic::BRLT, br_lt);
}
TEST_CASE("Pep9::Mnemonic::BREQ", "[scope:targets][kind:int][target:pep9]") {
  using Register = isa::Pep9::Register;
  inner(isa::Pep9::Mnemonic::BREQ, br_eq);
}
TEST_CASE("Pep9::Mnemonic::BRNE", "[scope:targets][kind:int][target:pep9]") {
  using Register = isa::Pep9::Register;
  inner(isa::Pep9::Mnemonic::BRNE, br_ne);
}
TEST_CASE("Pep9::Mnemonic::BRGE", "[scope:targets][kind:int][target:pep9]") {
  using Register = isa::Pep9::Register;
  inner(isa::Pep9::Mnemonic::BRGE, br_ge);
}
TEST_CASE("Pep9::Mnemonic::BRGT", "[scope:targets][kind:int][target:pep9]") {
  using Register = isa::Pep9::Register;
  inner(isa::Pep9::Mnemonic::BRGT, br_gt);
}
TEST_CASE("Pep9::Mnemonic::BRV", "[scope:targets][kind:int][target:pep9]") {
  using Register = isa::Pep9::Register;
  inner(isa::Pep9::Mnemonic::BRV, br_v);
}
TEST_CASE("Pep9::Mnemonic::BRC", "[scope:targets][kind:int][target:pep9]") {
  using Register = isa::Pep9::Register;
  inner(isa::Pep9::Mnemonic::BRC, br_c);
}
