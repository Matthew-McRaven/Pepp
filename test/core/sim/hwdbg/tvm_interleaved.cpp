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

#include "core/sim/debugger/tvm_interpreter.hpp"
#include "core/sim/debugger/tvm_tracebuffer.hpp"

TEST_CASE("tvm::Interpreter:  Interleaved submissions", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto mgr = std::make_shared<pepp::bts::BufferManager>();
  using namespace tvm::EncodedOp;
  using M = tvm::RegMask;
  tvm::TraceBuffer tb(mgr); // two initiators
  constexpr Device::ID S0{1}, S1{2};

  auto body_s0 = [&](auto enc) { tb.emit_body(S0, {enc.data(), enc.size()}); };
  auto body_s1 = [&](auto enc) { tb.emit_body(S1, {enc.data(), enc.size()}); };

  SECTION("Data is interleaved, code is not") {
    auto before = tb.cursor();

    // Both initiators recording simultaneously.
    tb.begin(S0);
    tb.begin(S1);

    // Interleave data writes: S0, S1, S0.
    auto d0a = tb.append_data(S0, std::array<u8, 2>{0xAA, 0xBB});
    auto d1 = tb.append_data(S1, std::array<u8, 2>{0xCC, 0xDD});
    auto d0b = tb.append_data(S0, std::array<u8, 2>{0xEE, 0xFF});

    // All three writes land in the same buffer, sequentially.
    CHECK(d0a.id == d1.id);
    CHECK(d1.id == d0b.id);
    // S0's second write is NOT adjacent to its first — S1's data sits between them.
    CHECK(d1.offset == d0a.offset + 2);
    CHECK(d0b.offset == d1.offset + 2);

    // Each initiator emits a distinguishable body.
    body_s0(LMR_of<false>(std::pair{M::MOD1_LO, u16(0xAAAA)}));
    body_s1(LMR_of<false>(std::pair{M::MOD1_LO, u16(0xBBBB)}));

    // End in reverse order to stress that code is not mixed.
    tb.commit(S1);
    tb.commit(S0);

    auto after = tb.cursor();

    // The location buffer has 2 entries.
    // Entry 0 is S1 (ended first), entry 1 is S0 (ended second).
    auto r = tb.range(before, after);
    auto it = r.begin();
    auto loc_s1 = *it;
    ++it;
    auto loc_s0 = *it;

    // Code is laid out sequentially: S1's subroutine precedes S0's.
    CHECK(loc_s1.id == loc_s0.id);
    CHECK(loc_s0.offset > loc_s1.offset);

    // Execute each subroutine independently — if code were interleaved,
    // these would produce wrong results or crash.
    tvm::Interpreter b0(mgr), b1(mgr);

    b1.run(loc_s1);
    CHECK(b1.stopped());
    CHECK(b1.regs().MOD1.lo == 0xBBBB);

    b0.run(loc_s0);
    CHECK(b0.stopped());
    CHECK(b0.regs().MOD1.lo == 0xAAAA);
  }

  SECTION("Per-initiator last_dp tracks independently") {
    tb.begin(S0);
    tb.begin(S1);

    auto d0 = tb.append_data(S0, std::array<u8, 4>{0x01, 0x02, 0x03, 0x04});
    auto d1 = tb.append_data(S1, std::array<u8, 4>{0x05, 0x06, 0x07, 0x08});

    // Each initiator's last_dp reflects only its own most recent write.
    CHECK(tb.last_dp(S0).id == d0.id);
    CHECK(tb.last_dp(S0).offset == d0.offset);
    CHECK(tb.last_dp(S1).id == d1.id);
    CHECK(tb.last_dp(S1).offset == d1.offset);

    // S0 writes again; only S0's last_dp advances.
    auto d0b = tb.append_data(S0, std::array<u8, 2>{0x09, 0x0A});
    CHECK(tb.last_dp(S0).offset == d0b.offset);
    CHECK(tb.last_dp(S1).offset == d1.offset); // unchanged

    body_s0(LDMOD1Lo{0x0000}.encode());
    body_s1(LDMOD1Lo{0x0000}.encode());
    tb.commit(S0);
    tb.commit(S1);
  }
}
