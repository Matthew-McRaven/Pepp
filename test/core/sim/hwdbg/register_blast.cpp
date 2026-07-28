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

#include "core/sim/debugger/register_blaster.hpp"

TEST_CASE("Basic RegisterBlaster", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto mgr = std::make_shared<pepp::bts::BufferManager>();
  auto ibuff = mgr->alloc_buffer();
  // Simplest possible program which sets a single register and halts.
  // Check that halt flag is set and that target register gains a value.
  SECTION("Can copy values into common registers") {
    RegisterBlaster blaster(mgr);
    ibuff->fill_clear(0);
    blaster.update_ip(ibuff->id());
    ibuff->append_packed(ldmod1lo(0x1234), halt());

    CHECK(blaster.csrs().L == 1);
    CHECK(blaster.csrs().M1 == 0);
    CHECK(blaster.regs().MOD1.lo == 0);
    REQUIRE_NOTHROW(blaster.step());
    CHECK(blaster.csrs().L == 1);
    CHECK(blaster.csrs().M1 == 1);
    CHECK(blaster.regs().MOD1.lo == 0x1234);
    REQUIRE_NOTHROW(blaster.step());
    CHECK(blaster.csrs().L == 0);
  }

  // Check that load masked register correctly re-orders the registers according to mask precedence.
  SECTION("Load multiple registers") {
    using M = RegisterBlaster::RegMask;
    RegisterBlaster blaster(mgr);
    ibuff->fill_clear(0);
    blaster.update_ip(ibuff->id());
    ibuff->append_packed(
        lmr_of<false>(
            std::pair{M::MOD1_LO, u16(0x1234)}, std::pair{M::ID_HI, u16(0xFEED)},
            std::pair{M::DP_LO,
                      u16(0xBEEF)}), // Mix up the order of registers to ensure lmr/lmr_of sort them correctly.
        halt());

    CHECK(blaster.csrs().L == 1);
    CHECK(blaster.csrs().M1 == 0);
    CHECK(blaster.regs().MOD1.lo == 0);
    CHECK(blaster.regs().ID.hi == 0);
    CHECK(blaster.regs().DP.lo == 0);
    REQUIRE_NOTHROW(blaster.step());
    CHECK(blaster.csrs().L == 1);
    CHECK(blaster.csrs().M1 == 1);
    CHECK(blaster.regs().MOD1.lo == 0x1234);
    CHECK(blaster.regs().ID.hi == 0xFEED);
    CHECK(blaster.regs().DP.lo == 0xBEEF);
    REQUIRE_NOTHROW(blaster.step());
    CHECK(blaster.csrs().L == 0);
  }

  // Test that branches are/not  taken by inserting a load register "under" the branch
  SECTION("Unconditional branch!") {
    using M = RegisterBlaster::RegMask;
    RegisterBlaster blaster(mgr);
    ibuff->fill_clear(0);
    blaster.update_ip(ibuff->id());
    ibuff->append_packed(br(0x6),                                         // Branch over the load register instruction.
                         lmr_of<false>(std::pair{M::DP_LO, u16(0xBEEF)}), // Hopefully not executed.
                         halt());

    CHECK(blaster.regs().DP.lo != 0xBEEF);
    REQUIRE_NOTHROW(blaster.step());
    CHECK(blaster.csrs().L == 1);
    CHECK(blaster.regs().DP.lo != 0xBEEF);
    REQUIRE_NOTHROW(blaster.step());
    CHECK(blaster.csrs().L == 0);
    CHECK(blaster.regs().DP.lo != 0xBEEF);
  }

  SECTION("Conditional branch (not taken)") {
    using M = RegisterBlaster::RegMask;
    RegisterBlaster blaster(mgr);
    ibuff->fill_clear(0);
    blaster.update_ip(ibuff->id());
    blaster.csrs().Z = 0;
    ibuff->append_packed(breq(0x6), lmr_of<false>(std::pair{M::DP_LO, u16(0xBEEF)}), halt());

    CHECK(blaster.regs().DP.lo != 0xBEEF);
    REQUIRE_NOTHROW(blaster.step());
    CHECK(blaster.regs().DP.lo != 0xBEEF);
    REQUIRE_NOTHROW(blaster.step());
    CHECK(blaster.regs().DP.lo == 0xBEEF);
  }

  SECTION("Conditional branch (taken)") {
    using M = RegisterBlaster::RegMask;
    RegisterBlaster blaster(mgr);
    ibuff->fill_clear(0);
    blaster.update_ip(ibuff->id());
    blaster.csrs().Z = 1;
    ibuff->append_packed(breq(0x6), lmr_of<false>(std::pair{M::DP_LO, u16(0xBEEF)}), halt());

    CHECK(blaster.regs().DP.lo != 0xBEEF);
    REQUIRE_NOTHROW(blaster.step());
    CHECK(blaster.regs().DP.lo != 0xBEEF);
    REQUIRE_NOTHROW(blaster.step());
    CHECK(blaster.regs().DP.lo != 0xBEEF);
  }
}
