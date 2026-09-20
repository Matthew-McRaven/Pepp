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

TEST_CASE("tvm::Interpreter:  Initiators taking turns", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto mgr = std::make_shared<pepp::bts::BufferManager>();
  using namespace tvm::EncodedOp;
  using M = tvm::RegMask;
  tvm::TraceBuffer tb(mgr); // two initiators, recording one at a time
  constexpr Device::ID S0{1}, S1{2};

  auto body_s0 = [&](auto enc) { tb.emit_body(S0, {enc.data(), enc.size()}); };
  auto body_s1 = [&](auto enc) { tb.emit_body(S1, {enc.data(), enc.size()}); };

  SECTION("Neither data nor code is interleaved") {
    auto before = tb.cursor();

    // The buffer takes one recording at a time, so two initiators sharing it take turns -- a multi-core system
    // stepping its cores in turn, rather than two of them running at once.
    tb.begin(S0);
    auto d0a = tb.append_data(S0, std::array<u8, 2>{0xAA, 0xBB});
    auto d0b = tb.append_data(S0, std::array<u8, 2>{0xEE, 0xFF});
    body_s0(LMR_of<false>(std::pair{M::MOD1_LO, u16(0xAAAA)}));
    const auto loc_s0 = tb.commit(S0);

    tb.begin(S1);
    auto d1 = tb.append_data(S1, std::array<u8, 2>{0xCC, 0xDD});
    body_s1(LMR_of<false>(std::pair{M::MOD1_LO, u16(0xBBBB)}));
    const auto loc_s1 = tb.commit(S1);

    // Each initiator still has its own data chain, so S1's payload lands in a different buffer entirely. That is what
    // keeps S0's two writes adjacent -- the adjacency a body needs to step DP with an address-free ACCDP rather than
    // an INCDP carrying a delta specific to how the turns fell.
    CHECK(d0a.id != d1.id);
    CHECK(d0b.id == d0a.id);
    CHECK(d0b.offset == d0a.offset + 2);

    // Entries and code are both laid out in commit order, which here is also the order they ran.
    auto r = tb.range(before, tb.cursor());
    auto it = r.begin();
    CHECK((*it).code.offset == loc_s0.code.offset);
    ++it;
    CHECK((*it).code.offset == loc_s1.code.offset);
    CHECK(loc_s1.code.id == loc_s0.code.id);
    CHECK(loc_s1.code.offset > loc_s0.code.offset);

    // Execute each subroutine independently — if code were interleaved, these would produce wrong results or crash.
    tvm::Interpreter b0(mgr, std::make_unique<tvm::ApplyBackend>(mgr)),
        b1(mgr, std::make_unique<tvm::ApplyBackend>(mgr));

    b1.run(loc_s1);
    CHECK(b1.stopped());
    CHECK(b1.regs().MOD1.lo == 0xBBBB);

    b0.run(loc_s0);
    CHECK(b0.stopped());
    CHECK(b0.regs().MOD1.lo == 0xAAAA);
  }

  SECTION("An aborted recording gives its entry back") {
    const auto before = tb.cursor();

    tb.begin(S0);
    body_s0(LMR_of<false>(std::pair{M::MOD1_LO, u16(0xDEAD)}));
    tb.abort(S0);

    tb.begin(S1);
    body_s1(LMR_of<false>(std::pair{M::MOD1_LO, u16(0xBEEF)}));
    const auto kept = tb.commit(S1);

    // Nothing was written for the abandoned ordinal, and it was the newest one, so abort() hands it back and the
    // range holds only the recording that committed.
    auto r = tb.range(before, tb.cursor());
    int entries = 0;
    for ([[maybe_unused]] auto program : r) entries++;
    CHECK(entries == 1);
    CHECK((*r.begin()).code.offset == kept.code.offset);

    tvm::Interpreter b(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    b.run(*r.begin());
    CHECK(b.stopped());
    CHECK(b.stop_cause() == tvm::StopCause::None);
    CHECK(b.regs().MOD1.lo == 0xBEEF);
  }
}
