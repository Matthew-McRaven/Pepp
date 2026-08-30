/*
 * Copyright (c) 2026. Stanley Warford, Matthew McRaven
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <bit>
#include <catch.hpp>
#include <cstdint>
#include "../api.hpp"
#include "core/arch/riscv/asmb/rv_mnemonics.hpp"
#include "core/arch/riscv/asmb/rvi_patterns.hpp"

using namespace rv32_test;

namespace {
// Scratch well clear of the program, which always starts at 0.
constexpr u32 SCRATCH = 0x1000;
} // namespace

TEST_CASE("RV32I loads and stores", "[scope:core][scope:core.sim][kind:unit][arch:rv]") {
  using namespace riscv;
  using enum riscv::ABIReg;
  auto [sys, mem, cpu] = make_cpu();
  u64 tick = 0;
  auto step = [&, cpu = cpu](std::size_t count) {
    for (std::size_t i = 0; i < count; ++i, ++tick) cpu->clock_tick(PulseSchedule::PulseIndex{tick}, tick);
  };
  cpu->write_register(riscv::ABIReg::sp, SCRATCH);

  SECTION("a value survives a store/load round trip at every width") {

    auto prog = {
        SW.encode(Values{.rs1 = u8(sp), .rs2 = u8(s6), .imm = 0}).bits(), // sw  s6, 0(sp)
        LW.encode(Values{.rs1 = u8(sp), .rd = u8(a0), .imm = 0}).bits(),  // lw  a0, 0(sp)
        SH.encode(Values{.rs1 = u8(sp), .rs2 = u8(s6), .imm = 0}).bits(), // sh  s6, 0(sp)
        LH.encode(Values{.rs1 = u8(sp), .rd = u8(a0), .imm = 0}).bits(),  // lh  a0, 0(sp)
        SB.encode(Values{.rs1 = u8(sp), .rs2 = u8(s6), .imm = 0}).bits(), // sb  s6, 0(sp)
        LB.encode(Values{.rs1 = u8(sp), .rd = u8(a0), .imm = 0}).bits(),  // lb  a0, 0(sp)

    };
    load_program(mem, prog);
    // Every byte distinct, so a width that reads too much or too little is visible in the result.
    // All three truncations stay positive, avoid sign extension
    cpu->write_register(riscv::ABIReg::s6, 0x12345678);

    // Execute pairs of store+load.
    step(2);
    CHECK(cpu->read_register(riscv::ABIReg::a0) == 0x12345678);
    step(2);
    CHECK(cpu->read_register(riscv::ABIReg::a0) == 0x00005678);
    step(2);
    CHECK(cpu->read_register(riscv::ABIReg::a0) == 0x00000078);
  }

  SECTION("narrow loads sign extend, their unsigned forms zero extend") {
    auto prog = {
        SB.encode(Values{.rs1 = u8(sp), .rs2 = u8(s6), .imm = 0}).bits(), // sb  s6, 0(sp)
        LB.encode(Values{.rs1 = u8(sp), .rd = u8(a0), .imm = 0}).bits(),  // lb  a0, 0(sp)
        LBU.encode(Values{.rs1 = u8(sp), .rd = u8(a1), .imm = 0}).bits(), // lbu a1, 0(sp)
        SH.encode(Values{.rs1 = u8(sp), .rs2 = u8(s6), .imm = 0}).bits(), // sh  s6, 0(sp)
        LH.encode(Values{.rs1 = u8(sp), .rd = u8(a0), .imm = 0}).bits(),  // lh  a0, 0(sp)
        LHU.encode(Values{.rs1 = u8(sp), .rd = u8(a1), .imm = 0}).bits(), // lhu a1, 0(sp)
    };
    load_program(mem, prog);
    // All ones, so the stored byte and halfword both have their high bit set.
    cpu->write_register(riscv::ABIReg::s6, 0xFFFFFFFF);

    // The signed and unsigned forms differ only in load<T>'s template argument, so swapping the
    // two would leave every other test in the suite passing.
    step(3);
    CHECK(cpu->read_register(riscv::ABIReg::a0) == 0xFFFFFFFF);
    CHECK(cpu->read_register(riscv::ABIReg::a1) == 0x000000FF);
    step(3);
    CHECK(cpu->read_register(riscv::ABIReg::a0) == 0xFFFFFFFF);
    CHECK(cpu->read_register(riscv::ABIReg::a1) == 0x0000FFFF);
  }

  SECTION("a narrow store leaves the neighbouring bytes alone") {
    auto prog = {
        SB.encode(Values{.rs1 = u8(sp), .rs2 = u8(s6), .imm = 0}).bits(), // sb  s6, 0(sp)
        SH.encode(Values{.rs1 = u8(sp), .rs2 = u8(s6), .imm = 0}).bits(), // sh  s6, 0(sp)
    };
    load_program(mem, prog);

    // The guest is little-endian, so byte 0 of the word is its least significant one: a correct sb
    // replaces only the low 8 bits, and a store that wrote a full word would take the rest with it.
    mem->write<u32, !bits::host_is_le>(SCRATCH, 0xAABBCCDD, rw);
    cpu->write_register(riscv::ABIReg::s6, 0x11);
    step(1);
    CHECK(mem->read<u32, !bits::host_is_le>(SCRATCH, rw).second == 0xAABBCC11);

    mem->write<u32, !bits::host_is_le>(SCRATCH, 0xAABBCCDD, rw);
    cpu->write_register(riscv::ABIReg::s6, 0x2233);
    step(1);
    CHECK(mem->read<u32, !bits::host_is_le>(SCRATCH, rw).second == 0xAABB2233);
  }

  SECTION("a negative offset addresses below the base register") {
    auto prog = {
        LW.encode(Values{.rs1 = u8(sp), .rd = u8(a0), .imm = -4}).bits(), // lw  a0, -4(sp)
    };
    load_program(mem, prog);
    // imm is 0xFFC as twelve bits; only signed_imm() sign extending it lands on SCRATCH rather
    // than SCRATCH + 4092.
    cpu->write_register(riscv::ABIReg::sp, SCRATCH + 4);
    mem->write<u32, !bits::host_is_le>(SCRATCH, 0xCAFEBABE, rw);
    mem->write<u32, !bits::host_is_le>(SCRATCH + 4, 0x0BADF00D, rw);

    step(1);
    CHECK(cpu->read_register(riscv::ABIReg::a0) == 0xCAFEBABE);
  }
}
