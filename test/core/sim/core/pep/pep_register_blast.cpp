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
  // Simplest possible program which sets a single register and halts.
  // Check that halt flag is set and that target register gains a value.
  SECTION("Validate that a system-created RegisterBlaster works") {
    auto blaster = sys->make_blaster();
    ibuff->fill_clear(0);
    blaster->update_ip(ibuff->id());
    ibuff->append_packed(LDMOD1Lo_1(0x1234).encode(), Halt_0().encode());

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
  SECTION("Compare accumulator") {
    auto blaster = sys->make_blaster();
    ibuff->fill_clear(0);
    auto scan = sys->register_scan();
    cpu->write_register(isa::Pep10::Register::A, 0xFEED);
    auto ref = *scan->find("A");
    auto loc = ibuff->location();
    // Condition code, register, field, data size, data[0], data[1].
    ibuff->append(ldpi_w(0xFEED));
    ibuff->append(cmpreg((u16)CC::E, ref.reg.value, ref.field.value));
    ibuff->append(Halt_0().encode());
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
}
