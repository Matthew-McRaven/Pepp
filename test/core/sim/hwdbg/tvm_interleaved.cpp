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

#include "core/sim/debugger/tvm_apply_backend.hpp"
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

  SECTION("Neither data nor code is interleaved") {
    auto before = tb.cursor();

    // Both initiators recording simultaneously.
    tb.begin(S0);
    tb.begin(S1);

    // Interleave the calls: S0, S1, S0.
    auto d0a = tb.append_data(S0, std::array<u8, 2>{0xAA, 0xBB});
    auto d1 = tb.append_data(S1, std::array<u8, 2>{0xCC, 0xDD});
    auto d0b = tb.append_data(S0, std::array<u8, 2>{0xEE, 0xFF});

    // Each initiator has its own chain, so S1's payload lands in a different buffer entirely.
    CHECK(d0a.id != d1.id);
    // S0's second write stays adjacent to its first despite S1's call in between. That adjacency is the whole point:
    // it lets the body step DP with an address-free ACCDP instead of an INCDP carrying an interleaving-specific
    // delta, which is what keeps the body byte-identical to other programs and therefore templatizable.
    CHECK(d0b.id == d0a.id);
    CHECK(d0b.offset == d0a.offset + 2);

    // Each initiator emits a distinguishable body.
    body_s0(LMR_of<false>(std::pair{M::MOD1_LO, u16(0xAAAA)}));
    body_s1(LMR_of<false>(std::pair{M::MOD1_LO, u16(0xBBBB)}));

    // End in reverse order to stress that code is not mixed.
    tb.commit(S1);
    tb.commit(S0);

    auto after = tb.cursor();

    // The location buffer has 2 entries, in *begin* order: S0 reserved index 0 before S1 reserved index 1, whatever
    // order they finished in. Reserving at begin() is what stops an entry from ever landing behind a cursor a
    // consumer has already read past.
    auto r = tb.range(before, after);
    auto it = r.begin();
    auto loc_s0 = *it;
    ++it;
    auto loc_s1 = *it;

    // Code is laid out in *commit* order, which here is the opposite: S1 finished first, so its subroutine was
    // appended to the chain first. The two orders being independent is the point -- the index is claimed at begin(),
    // the bytes it names are written at commit().
    CHECK(loc_s1.code.id == loc_s0.code.id);
    CHECK(loc_s0.code.offset > loc_s1.code.offset);

    // Execute each subroutine independently — if code were interleaved,
    // these would produce wrong results or crash.
    tvm::Interpreter b0(mgr, std::make_unique<tvm::ApplyBackend>(mgr)),
        b1(mgr, std::make_unique<tvm::ApplyBackend>(mgr));

    b1.run(loc_s1);
    CHECK(b1.stopped());
    CHECK(b1.regs().MOD1.lo == 0xBBBB);

    b0.run(loc_s0);
    CHECK(b0.stopped());
    CHECK(b0.regs().MOD1.lo == 0xAAAA);
  }

  SECTION("A recording open across a slot advance stays in the slot it started in") {
    // The regression this pins down: S0 opens, S1 fills the rest of the slot and pushes the head forward, and S0 then
    // commits. Before slots were pinned at begin(), S0's later payloads went to the new slot while its location entry
    // still named the old one -- so acknowledging the old slot freed the payload out from under a live record.
    tb.begin(S0);
    const auto d0 = tb.append_data(S0, std::array<u8, 2>{0x11, 0x22});
    body_s0(LMR_of<false>(std::pair{M::MOD1_LO, u16(0x5A5A)}));

    // S0 holds index 0, so S1 needs one fewer than the slot's capacity to take the rest of it. commit() will not
    // advance out from under an open recording, so the head is still on slot 0 afterwards.
    for (u16 i = 0; i + 1 < tvm::TraceBuffer::MAX_LOCATION_ENTRIES; ++i) {
      tb.begin(S1);
      tb.commit(S1);
    }
    REQUIRE(tb.cursor().slot == 0);

    // This is the begin() that has to advance, because nothing else could.
    tb.begin(S1);
    REQUIRE(tb.cursor().slot == 1);
    tb.commit(S1);

    // Slot 0 is behind the head and fully committed apart from S0 -- but S0 is still writing to it, so reclaiming it
    // has to wait.
    tb.acknowledge({1, 0});
    CHECK(tb.ring_occupancy() > 0.0f);

    const auto loc = tb.commit(S0);
    // Everything S0 wrote is in the slot it started in, and the payload is still live.
    CHECK(loc.data.id == d0.id);
    CHECK(loc.data.offset == d0.offset);
    REQUIRE(mgr->find(loc.data.id) != nullptr);
    // And it is the entry S0 reserved before any of S1's.
    const auto entry0 = *tb.range(tvm::Cursor{0, 0}, tvm::Cursor{0, 1}).begin();
    CHECK(entry0.code.id == loc.code.id);
    CHECK(entry0.code.offset == loc.code.offset);
    CHECK(entry0.data.id == loc.data.id);

    tvm::Interpreter b(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    b.run(loc);
    CHECK(b.stopped());
    CHECK(b.csrs().F == 0);
    CHECK(b.regs().MOD1.lo == 0x5A5A);

    // With S0 closed, the slot can go.
    tb.acknowledge({1, 0});
    CHECK(tb.ring_occupancy() == Catch::Approx(0.0f));
  }

  SECTION("An aborted recording leaves a runnable no-op in the entry it reserved") {
    const auto before = tb.cursor();

    tb.begin(S0);
    body_s0(LMR_of<false>(std::pair{M::MOD1_LO, u16(0xDEAD)}));
    tb.abort(S0);

    tb.begin(S1);
    body_s1(LMR_of<false>(std::pair{M::MOD1_LO, u16(0xBEEF)}));
    tb.commit(S1);

    // The abandoned index is still an entry -- begin() reserved it, so it is inside the cursor range either way.
    auto it = tb.range(before, tb.cursor()).begin();
    const auto aborted = *it;
    ++it;
    const auto kept = *it;

    tvm::Interpreter b(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    // A zeroed entry would hard-stop on Buffer::ID{0}, and run_each breaks on a hard stop -- so one aborted
    // instruction would end the whole replay. Pointing it at a bare HALT makes it a no-op instead.
    b.run(aborted);
    CHECK(b.stopped());
    CHECK(b.csrs().F == 0);
    CHECK(b.stop_cause() == tvm::StopCause::None);
    // The aborted body never ran.
    CHECK(b.regs().MOD1.lo == 0);

    b.run(kept);
    CHECK(b.stopped());
    CHECK(b.regs().MOD1.lo == 0xBEEF);
  }
}
