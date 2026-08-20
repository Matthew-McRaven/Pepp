/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
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

template <typename Register, typename CSR, typename Mnemonic>
void inner(PepISA3CPU::ISA isa, Mnemonic op, should_branch taken) {
  auto [sys, mem, cpu] = make_cpu(isa);
  // Loop over a subset of possible values for the target register.
  u16 opspec = 0xfeed;

  // Object code for instruction under test.
  auto program =
      std::array<u8, 3>{(u8)op, static_cast<uint8_t>((opspec >> 8) & 0xff), static_cast<uint8_t>(opspec & 0xff)};
  REQUIRE_NOTHROW(mem->write(0, {program.data(), program.size()}, rw));
  for (u8 nzvc = 0; nzvc < 0b1'00'00; nzvc++) {
    cpu->registers()->clear(0);
    cpu->write_packed_csr(nzvc);

    REQUIRE_NOTHROW(cpu->clock_tick(PulseSchedule::PulseIndex{0}, 0));
    auto [n, z, v, c] = PepCSRBank::unpack(cpu->read_packed_csr());

    CHECK(reg(cpu, Register::A) == 0);
    CHECK(reg(cpu, Register::X) == 0);
    CHECK(reg(cpu, Register::IS) == (u8)op);
    // OS loaded the Mem[0x0001-0x0002].
    bool branch_taken = false;
    CHECK(reg(cpu, Register::OS) == opspec);
    if (taken(n, z, v, c)) {
      CHECK(reg(cpu, Register::PC) == opspec);
    } else {
      CHECK(reg(cpu, Register::PC) == 0x3);
    }
  }
}

} // namespace

TEST_CASE("(new) Pep/10, BR, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, MN::BR, br_unconditional);
}
TEST_CASE("(new) Pep/10, BRLE, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, MN::BRLE, br_le);
}
TEST_CASE("(new) Pep/10, BRLT, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, MN::BRLT, br_lt);
}
TEST_CASE("(new) Pep/10, BREQ, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, MN::BREQ, br_eq);
}
TEST_CASE("(new) Pep/10, BRNE, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, MN::BRNE, br_ne);
}
TEST_CASE("(new) Pep/10, BRGE, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, MN::BRGE, br_ge);
}
TEST_CASE("(new) Pep/10, BRGT, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, MN::BRGT, br_gt);
}
TEST_CASE("(new) Pep/10, BRV, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, MN::BRV, br_v);
}
TEST_CASE("(new) Pep/10, BRC, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, MN::BRC, br_c);
}

TEST_CASE("(new) Pep/9, BR, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep9::Register;
  using CSR = isa::Pep9::CSR;
  using MN = isa::Pep9::Mnemonic;
  inner<Register, CSR, MN>(PepISA3CPU::ISA::Pep9, MN::BR, br_unconditional);
}
TEST_CASE("(new) Pep/9, BRLE, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep9::Register;
  using CSR = isa::Pep9::CSR;
  using MN = isa::Pep9::Mnemonic;
  inner<Register, CSR, MN>(PepISA3CPU::ISA::Pep9, MN::BRLE, br_le);
}
TEST_CASE("(new) Pep/9, BRLT, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep9::Register;
  using CSR = isa::Pep9::CSR;
  using MN = isa::Pep9::Mnemonic;
  inner<Register, CSR, MN>(PepISA3CPU::ISA::Pep9, MN::BRLT, br_lt);
}
TEST_CASE("(new) Pep/9, BREQ, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep9::Register;
  using CSR = isa::Pep9::CSR;
  using MN = isa::Pep9::Mnemonic;
  inner<Register, CSR, MN>(PepISA3CPU::ISA::Pep9, MN::BREQ, br_eq);
}
TEST_CASE("(new) Pep/9, BRNE, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep9::Register;
  using CSR = isa::Pep9::CSR;
  using MN = isa::Pep9::Mnemonic;
  inner<Register, CSR, MN>(PepISA3CPU::ISA::Pep9, MN::BRNE, br_ne);
}
TEST_CASE("(new) Pep/9, BRGE, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep9::Register;
  using CSR = isa::Pep9::CSR;
  using MN = isa::Pep9::Mnemonic;
  inner<Register, CSR, MN>(PepISA3CPU::ISA::Pep9, MN::BRGE, br_ge);
}
TEST_CASE("(new) Pep/9, BRGT, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep9::Register;
  using CSR = isa::Pep9::CSR;
  using MN = isa::Pep9::Mnemonic;
  inner<Register, CSR, MN>(PepISA3CPU::ISA::Pep9, MN::BRGT, br_gt);
}
TEST_CASE("(new) Pep/9, BRV, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep9::Register;
  using CSR = isa::Pep9::CSR;
  using MN = isa::Pep9::Mnemonic;
  inner<Register, CSR, MN>(PepISA3CPU::ISA::Pep9, MN::BRV, br_v);
}
TEST_CASE("(new) Pep/9, BRC, i", "[scope:core][scope:core.sim][kind:unit][arch:pep10]") {
  using Register = isa::Pep9::Register;
  using CSR = isa::Pep9::CSR;
  using MN = isa::Pep9::Mnemonic;
  inner<Register, CSR, MN>(PepISA3CPU::ISA::Pep9, MN::BRC, br_c);
}