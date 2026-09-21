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

TEST_CASE("(new) Pep/10, SCALL", "[scope:core][scope:core.sim][kind:instr][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using MN = isa::Pep10::Mnemonic;
  auto [sys, mem, cpu] = make_cpu(PepISA3CPU::ISA::Pep10);
  const auto program = std::array<u8, 3>{(u8)MN::SCALL, 0xCA, 0xBE};
  REQUIRE_NOTHROW(mem->write(0, {program.data(), program.size()}, rw));
  const auto os_sp = std::array<u8, 2>{0x80, 0x86};
  REQUIRE_NOTHROW(mem->write((u16)isa::Pep10::MemoryVectors::SystemStackPtr, {os_sp.data(), os_sp.size()}, rw));

  cpu->registers()->clear(0);
  cpu->csrs()->clear(0);
  cpu->write_packed_csr(0b1101);
  cpu->write_register(Register::A, 0x1122);
  cpu->write_register(Register::X, 0xBAAD);
  cpu->write_register(Register::SP, 0xFEED);

  REQUIRE_NOTHROW(cpu->clock_tick(PulseIndex{0}, 0));

  const auto frame = std::array<u8, 12>{/*NZVC*/ 0b1101,
                                        /*A*/ 0x11,           0x22,
                                        /*X*/ 0xBA,           0xAD,
                                        /*PC*/ 0x00,          0x03,
                                        /*SP*/ 0xFE,          0xED,
                                        /*IS*/ (u8)MN::SCALL,
                                        /*OS*/ 0xCA,          0xBE};
  const auto sp = reg(cpu, Register::SP);
  CHECK(sp + frame.size() == 0x8086);
  std::array<u8, 12> actual{};
  REQUIRE_NOTHROW(mem->read(sp, {actual.data(), actual.size()}, rw));
  for (std::size_t it = 0; it < frame.size(); ++it) CHECK(actual[it] == frame[it]);
}

TEST_CASE("(new) Pep/10, SRET", "[scope:core][scope:core.sim][kind:instr][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using MN = isa::Pep10::Mnemonic;
  auto [sys, mem, cpu] = make_cpu(PepISA3CPU::ISA::Pep10);
  const auto program = std::array<u8, 1>{(u8)MN::SRET};
  REQUIRE_NOTHROW(mem->write(0, {program.data(), program.size()}, rw));

  const auto frame = std::array<u8, 10>{/*NZVC*/ 0b1101,
                                        /*A*/ 0x11,          0x22,
                                        /*X*/ 0xBA,          0xAD,
                                        /*PC*/ 0xCA,         0xDE,
                                        /*SP*/ 0xFE,         0xED,
                                        /*IS*/ (u8)MN::SCALL};
  const u16 frame_at = 0x8086 - 12;
  REQUIRE_NOTHROW(mem->write(frame_at, {frame.data(), frame.size()}, rw));

  cpu->registers()->clear(0);
  cpu->csrs()->clear(0);
  cpu->write_register(Register::SP, frame_at);

  REQUIRE_NOTHROW(cpu->clock_tick(PulseIndex{0}, 0));

  CHECK(cpu->read_packed_csr() == frame[0]);
  CHECK(reg(cpu, Register::A) == 0x1122);
  CHECK(reg(cpu, Register::X) == 0xBAAD);
  CHECK(reg(cpu, Register::PC) == 0xCADE);
  CHECK(reg(cpu, Register::SP) == 0xFEED);

  std::array<u8, 2> os_sp{};
  REQUIRE_NOTHROW(mem->read((u16)isa::Pep10::MemoryVectors::SystemStackPtr, {os_sp.data(), os_sp.size()}, rw));
  CHECK((os_sp[0] << 8 | os_sp[1]) == 0x8086);
}
