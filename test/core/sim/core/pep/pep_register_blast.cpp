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

#include "./instr/api.hpp"
#include "core/sim/debugger/register_blaster.hpp"

TEST_CASE("Access registers from RegisterBlaster", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  using CC = tvm::ConditionCode;
  using namespace tvm::EncodedOp;
  auto [sys, mem, cpu] = make_cpu(PepISA3CPU::ISA::Pep10);
  auto bufmgr = sys->buffer_manager();
  auto ibuff = bufmgr->alloc_buffer();
  auto dbuff = bufmgr->alloc_buffer();
  // Simplest possible program which sets a single register and halts.
  // Check that halt flag is set and that target register gains a value.
  SECTION("Validate that a system-created RegisterBlaster works") {
    auto blaster = sys->make_blaster();
    ibuff->fill_clear(0);
    blaster->update_ip(ibuff->id());
    ibuff->append_packed(LDMOD1Lo(0x1234).encode(), Halt<0>().encode());

    CHECK(blaster->csrs().L == 1);
    CHECK(blaster->csrs().M1 == 0);
    CHECK(blaster->regs().MOD1.lo == 0);
    REQUIRE_NOTHROW(blaster->step());
    CHECK(blaster->csrs().L == 1);
    CHECK(blaster->csrs().M1 == 1);
    CHECK(blaster->regs().MOD1.lo == 0x1234);
    REQUIRE_NOTHROW(blaster->step());
    CHECK(blaster->csrs().L == 0);
  }
  // Simplest possible program which sets a single register and halts.
  // Check that halt flag is set and that target register gains a value.
  SECTION("Compare accumulator (immediate)") {
    auto blaster = sys->make_blaster();
    ibuff->fill_clear(0);
    auto scan = sys->register_scan();
    cpu->write_register(isa::Pep10::Register::A, 0xFEED);
    auto ref = *scan->find("A");
    auto loc = ibuff->location();
    // Register, field, data size, data words.
    ibuff->append(CmpReg<3>(ref.reg.value, ref.field.value).encode(0xFEED));
    ibuff->append(Halt<0>().encode());
    // Before execution, system should be live with the z-bit unset
    CHECK(blaster->csrs().L == 1);
    CHECK(blaster->csrs().F == 0);
    CHECK(blaster->csrs().Z == 0);
    // After comparison, Z bit should be set.
    blaster->run_direct(loc);
    CHECK(blaster->csrs().L == 0);
    CHECK(blaster->csrs().F == 0);
    CHECK(blaster->csrs().Z == 1);
  }
  // Simplest possible program which sets a single register and halts.
  // Check that halt flag is set and that target register gains a value.
  SECTION("Compare accumulator / X (DP)") {
    auto blaster = sys->make_blaster();
    ibuff->fill_clear(0);
    auto scan = sys->register_scan();
    cpu->write_register(isa::Pep10::Register::A, 0xFEED);
    cpu->write_register(isa::Pep10::Register::X, 0xBEEF);
    auto a = *scan->find("A");
    auto x = *scan->find("X");
    auto loc1 = ibuff->location();
    // Have to store data in little-endian format, because VM is LE...
    dbuff->append(std::array<u8, 2>{0xED, 0xFE});
    dbuff->append(std::array<u8, 2>{0xEF, 0xBE});
    // First program comparse A to FEED. Should set z bit=1
    ibuff->append(LDP<3>(tvm::SegmentPair{.hi = dbuff->id().value, .lo = 0}, 2).encode());
    // Use non-immediate variant, which
    ibuff->append(CmpReg<2>(a.reg.value, a.field.value).encode());
    ibuff->append(Halt<0>().encode());
    // The second program compares X to BEEF. Should set z bit=1
    auto loc2 = ibuff->location();
    // Rather than form a new DP triple, use one of the incrementing opcodes!
    ibuff->append(ACCDP(2).encode());
    ibuff->append(CmpReg<2>(x.reg.value, x.field.value).encode());
    ibuff->append(Halt<0>().encode());
    // Before execution, system should be live with the z-bit unset
    CHECK(blaster->csrs().L == 1);
    CHECK(blaster->csrs().F == 0);
    CHECK(blaster->csrs().Z == 0);
    // After comparison, Z bit should be set.
    blaster->run_direct(loc1);
    CHECK(blaster->csrs().L == 0);
    CHECK(blaster->csrs().F == 0);
    CHECK(blaster->csrs().Z == 1);
    // Force-clear Z to ensure that the next program sets it again.
    blaster->csrs().Z = 0;
    blaster->run_direct(loc2);
    CHECK(blaster->csrs().L == 0);
    CHECK(blaster->csrs().F == 0);
    CHECK(blaster->csrs().Z == 1);
  }

  SECTION("Set, compare, and clear memory") {
    auto blaster = sys->make_blaster();
    ibuff->fill_clear(0);
    const u32 offset = 0xFEED;
    const u16 val = 0xBEEF;

    auto loc1 = ibuff->location();
    // Data for mem ops is stored in /whatever/ order you provided it.
    dbuff->append(std::array<u8, 2>{0xBE, 0xEF});
    // First program comparse A to FEED. Should set z bit=1
    ibuff->append(LDP<3>(tvm::SegmentPair{.hi = dbuff->id().value, .lo = 0}, 2).encode());
    // Use non-immediate variant, which
    ibuff->append(
        SetMem<false, 4>{.access = rw.as_u8(), .dev = mem->id().value, .off = SegmentPair{.hi = 0, .lo = offset}}
            .encode());
    ibuff->append(CmpMem<3>{.dev = mem->id().value, .off = SegmentPair{.hi = 0, .lo = offset}}.encode());
    ibuff->append(Halt<0>().encode());
    auto loc2 = ibuff->location();
    ibuff->append(ClrMem<1>{.dev = mem->id().value}.encode());
    ibuff->append(Halt<0>().encode());

    // Before execution, system should be live with the z-bit unset
    CHECK(blaster->csrs().L == 1);
    CHECK(blaster->csrs().F == 0);
    CHECK(blaster->csrs().Z == 0);
    // Memory value should not-yet be set
    CHECK(((Target *)mem)->read<u16, bits::host_is_le>(offset, rw).second == 0x0000);
    // After comparison, Z bit should be set.
    blaster->run_direct(loc1);
    CHECK(blaster->csrs().L == 0);
    CHECK(blaster->csrs().F == 0);
    CHECK(blaster->csrs().Z == 1);
    CHECK(((Target *)mem)->read<u16, bits::host_is_le>(offset, rw).second == val);
    // Submit 2nd program which clears memory.
    blaster->run_direct(loc2);
    CHECK(blaster->csrs().L == 0);
    CHECK(blaster->csrs().F == 0);
    CHECK(((Target *)mem)->read<u16, bits::host_is_le>(offset, rw).second == 0x0000);
  }
}
