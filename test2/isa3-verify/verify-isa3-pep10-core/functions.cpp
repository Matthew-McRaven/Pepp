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

template <typename Register, typename CSR, typename Mnemonic> void inner_ret(PepISA3CPU::ISA isa, Mnemonic op) {
  auto [sys, mem, cpu] = make_cpu(isa);
  // Object code for instruction under test.
  auto program = std::array<u8, 1>{(u8)op};
  REQUIRE_NOTHROW(mem->write(0, {program.data(), program.size()}, rw));
  const u16 init_sp = 0xFEED;
  const u16 end_pc = 0xDEAD;

  cpu->registers()->clear(0);
  cpu->csrs()->clear(0);
  cpu->write_register(Register::SP, init_sp);
  ((Target *)mem)->write<u16, bits::host_is_le>(init_sp, end_pc, rw);

  REQUIRE_NOTHROW(cpu->clock_tick(PulseSchedule::PulseIndex{0}, 0));

  CHECK(reg(cpu, Register::PC) == end_pc);
  CHECK(reg(cpu, Register::SP) == init_sp + 2);
  CHECK(reg(cpu, Register::IS) == (u8)op);
  CHECK(reg(cpu, Register::OS) == 0);
  CHECK(reg(cpu, Register::X) == 0);
  CHECK(reg(cpu, Register::A) == 0);
}

template <typename Register, typename CSR, typename Mnemonic> void inner_call(PepISA3CPU::ISA isa, Mnemonic op) {
  auto [sys, mem, cpu] = make_cpu(isa);
  // Object code for instruction under test.
  auto program = std::array<u8, 3>{(u8)op, 0xDE, 0xAD};
  REQUIRE_NOTHROW(mem->write(0, {program.data(), program.size()}, rw));
  const u16 init_sp = 0xFEED;
  const u16 end_pc = 0xDEAD;

  cpu->registers()->clear(0);
  cpu->csrs()->clear(0);
  cpu->write_register(Register::SP, init_sp);

  REQUIRE_NOTHROW(cpu->clock_tick(PulseSchedule::PulseIndex{0}, 0));

  CHECK(reg(cpu, Register::PC) == end_pc);
  CHECK(reg(cpu, Register::SP) == init_sp - 2);
  const auto sp = reg(cpu, Register::SP);
  const auto mem_sp = ((Target *)mem)->read<u16, bits::host_is_le>(sp, rw).second;
  CHECK(mem_sp == 0x0003); // The return address is 0x0003, since that was the next PC prior to call.
  CHECK(reg(cpu, Register::IS) == (u8)op);
  CHECK(reg(cpu, Register::OS) == 0xDEAD);
  CHECK(reg(cpu, Register::X) == 0);
  CHECK(reg(cpu, Register::A) == 0);
}

} // namespace

TEST_CASE("(new) Pep/10, RET", "[scope:core][scope:core.sim][kind:instr][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner_ret<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, MN::RET);
}
TEST_CASE("(new) Pep/9, RET", "[scope:core][scope:core.sim][kind:instr][arch:pep10]") {
  using Register = isa::Pep9::Register;
  using CSR = isa::Pep9::CSR;
  using MN = isa::Pep9::Mnemonic;
  inner_ret<Register, CSR, MN>(PepISA3CPU::ISA::Pep9, MN::RET);
}

TEST_CASE("(new) Pep/10, CALL, i", "[scope:core][scope:core.sim][kind:instr][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner_call<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, MN::CALL);
}
TEST_CASE("(new) Pep/9, CALL, i", "[scope:core][scope:core.sim][kind:instr][arch:pep10]") {
  using Register = isa::Pep9::Register;
  using CSR = isa::Pep9::CSR;
  using MN = isa::Pep9::Mnemonic;
  inner_call<Register, CSR, MN>(PepISA3CPU::ISA::Pep9, MN::CALL);
}
