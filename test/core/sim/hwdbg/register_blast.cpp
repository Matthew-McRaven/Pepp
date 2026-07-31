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
#include "core/sim/debugger/tvm_tracebuffer.hpp"

TEST_CASE("Basic RegisterBlaster", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto mgr = std::make_shared<pepp::bts::BufferManager>();
  using namespace tvm::EncodedOp;
  using M = tvm::RegMask;
  tvm::TraceBuffer tb(mgr, 1);
  constexpr u16 S = 0;

  auto prefix = [&](auto enc) { tb.emit_prefix(S, {enc.data(), enc.size()}); };
  auto body = [&](auto enc) { tb.emit_body(S, {enc.data(), enc.size()}); };

  SECTION("Can copy values into common registers") {
    RegisterBlaster blaster(mgr);
    auto before = tb.cursor();

    tb.begin(S);
    body(LDMOD1Lo{0x1234}.encode());
    tb.end(S);

    CHECK(!blaster.stopped());
    CHECK(blaster.csrs().M1 == 0);
    CHECK(blaster.regs().MOD1.lo == 0);
    for (auto loc : tb.range(before, tb.cursor()))
      blaster.run_direct(loc);
    CHECK(blaster.stopped());
    CHECK(blaster.csrs().M1 == 1);
    CHECK(blaster.regs().MOD1.lo == 0x1234);
  }

  SECTION("Load multiple registers") {
    RegisterBlaster blaster(mgr);
    auto before = tb.cursor();

    tb.begin(S);
    body(LMR_of<false>(std::pair{M::MOD1_LO, u16(0x1234)}, std::pair{M::ID_HI, u16(0xFEED)},
                        std::pair{M::DP_LO, u16(0xBEEF)}));
    tb.end(S);

    CHECK(!blaster.stopped());
    CHECK(blaster.csrs().M1 == 0);
    CHECK(blaster.regs().MOD1.lo == 0);
    CHECK(blaster.regs().ID.hi == 0);
    CHECK(blaster.regs().DP.lo == 0);
    for (auto loc : tb.range(before, tb.cursor()))
      blaster.run_direct(loc);
    CHECK(blaster.stopped());
    CHECK(blaster.stop_cause() == StopCause::None);
    CHECK(blaster.csrs().M1 == 1);
    CHECK(blaster.regs().MOD1.lo == 0x1234);
    CHECK(blaster.regs().ID.hi == 0xFEED);
    CHECK(blaster.regs().DP.lo == 0xBEEF);
  }

  // Branch tests: the body contains BR + LMR + (HALT appended by TB).
  // BR<1>(0x6) jumps over the 6-byte LMR to land on the HALT.
  SECTION("Unconditional branch!") {
    RegisterBlaster blaster(mgr);
    auto before = tb.cursor();

    tb.begin(S);
    body(BR<1>{0x6}.encode());
    body(LMR_of<false>(std::pair{M::DP_LO, u16(0xBEEF)}));
    tb.end(S);

    CHECK(blaster.regs().DP.lo != 0xBEEF);
    for (auto loc : tb.range(before, tb.cursor()))
      blaster.run_direct(loc);
    CHECK(blaster.regs().DP.lo != 0xBEEF);
    CHECK(blaster.stopped());
    CHECK(blaster.stop_cause() == StopCause::None);
  }

  SECTION("Conditional branch (not taken)") {
    RegisterBlaster blaster(mgr);
    blaster.csrs().Z = 0;
    auto before = tb.cursor();

    tb.begin(S);
    body(BREQ<1>{0x6}.encode());
    body(LMR_of<false>(std::pair{M::DP_LO, u16(0xBEEF)}));
    tb.end(S);

    CHECK(blaster.regs().DP.lo != 0xBEEF);
    for (auto loc : tb.range(before, tb.cursor()))
      blaster.run_direct(loc);
    // Branch not taken: LMR executed, DP.lo set.
    CHECK(blaster.regs().DP.lo == 0xBEEF);
    CHECK(blaster.stopped());
  }

  SECTION("Conditional branch (taken)") {
    RegisterBlaster blaster(mgr);
    blaster.csrs().Z = 1;
    auto before = tb.cursor();

    tb.begin(S);
    body(BREQ<1>{0x6}.encode());
    body(LMR_of<false>(std::pair{M::DP_LO, u16(0xBEEF)}));
    tb.end(S);

    CHECK(blaster.regs().DP.lo != 0xBEEF);
    for (auto loc : tb.range(before, tb.cursor()))
      blaster.run_direct(loc);
    // Branch taken: LMR skipped, DP.lo unchanged.
    CHECK(blaster.regs().DP.lo != 0xBEEF);
    CHECK(blaster.stopped());
    CHECK(blaster.stop_cause() == StopCause::None);
  }
}
