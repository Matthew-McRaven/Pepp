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

TEST_CASE("DP update modes", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto mgr = std::make_shared<pepp::bts::BufferManager>();
  using namespace tvm::EncodedOp;

  SECTION("LDP: absolute load for first data access") {
    tvm::TraceBuffer tb(mgr, 1);
    constexpr u16 S = 0;
    RegisterBlaster blaster(mgr);
    auto before = tb.cursor();

    tb.begin(S);
    auto d = tb.append_data(S, std::array<u8, 4>{0xAA, 0xBB, 0xCC, 0xDD});
    auto ldp = LDP<3>(SegmentPair{.hi = (u16)d.id.value, .lo = d.offset}, 4).encode();
    tb.emit_prefix(S, {ldp.data(), ldp.size()});
    tb.end(S);

    for (auto loc : tb.range(before, tb.cursor()))
      blaster.run_direct(loc);
    CHECK(blaster.regs().DP.hi == d.id.value);
    CHECK(blaster.regs().DP.lo == d.offset);
    CHECK(blaster.regs().DS == 4);
  }

  SECTION("ACCDP: advance by previous DS for tightly packed data") {
    tvm::TraceBuffer tb(mgr, 1);
    constexpr u16 S = 0;
    RegisterBlaster blaster(mgr);
    auto before = tb.cursor();

    // Program 1: LDP to first chunk.
    tb.begin(S);
    auto d1 = tb.append_data(S, std::array<u8, 2>{0x11, 0x22});
    auto ldp = LDP<3>(SegmentPair{.hi = (u16)d1.id.value, .lo = d1.offset}, 2).encode();
    tb.emit_prefix(S, {ldp.data(), ldp.size()});
    tb.end(S);

    // Program 2: ACCDP to second chunk (immediately follows d1).
    tb.begin(S);
    auto d2 = tb.append_data(S, std::array<u8, 4>{0x33, 0x44, 0x55, 0x66});
    // d2 is tightly packed after d1.
    CHECK(d2.offset == d1.offset + 2);
    auto accdp = ACCDP{4}.encode();
    tb.emit_prefix(S, {accdp.data(), accdp.size()});
    tb.end(S);

    auto r = tb.range(before, tb.cursor());
    auto it = r.begin();
    blaster.run_direct(*it);
    CHECK(blaster.regs().DP.hi == d1.id.value);
    CHECK(blaster.regs().DP.lo == d1.offset);
    CHECK(blaster.regs().DS == 2);

    ++it;
    blaster.run_direct(*it);
    // DP.lo advanced by old DS=2; DP.hi unchanged.
    CHECK(blaster.regs().DP.hi == d1.id.value);
    CHECK(blaster.regs().DP.lo == d2.offset);
    CHECK(blaster.regs().DS == 4);
  }

  SECTION("INCDP: explicit increment for non-contiguous data") {
    // Two submitters interleave data, creating a gap in submitter A's writes.
    tvm::TraceBuffer tb(mgr, 2);
    constexpr u16 A = 0, B = 1;
    RegisterBlaster blaster(mgr);
    auto before = tb.cursor();

    tb.begin(A);
    tb.begin(B);

    auto da1 = tb.append_data(A, std::array<u8, 2>{0x11, 0x22});
    tb.append_data(B, std::array<u8, 2>{0xFF, 0xFF}); // B's data sits between A's writes
    auto da2 = tb.append_data(A, std::array<u8, 2>{0x33, 0x44});

    // A's second chunk is 4 bytes past its first (B's 2 bytes in between).
    CHECK(da2.offset == da1.offset + 4);

    // End B first so its entry is out of the way.
    tb.end(B);

    // A program 1: LDP to A's first chunk.
    auto ldp = LDP<3>(SegmentPair{.hi = (u16)da1.id.value, .lo = da1.offset}, 2).encode();
    tb.emit_prefix(A, {ldp.data(), ldp.size()});
    tb.end(A);

    // A program 2: INCDP past B's data to reach A's second chunk.
    // ACCDP would advance by old DS=2, landing on B's data. Wrong.
    // INCDP(4, 2) advances by 4, landing on da2. Correct.
    tb.begin(A);
    auto incdp = INCDP{4, 2}.encode();
    tb.emit_prefix(A, {incdp.data(), incdp.size()});
    tb.end(A);

    auto r = tb.range(before, tb.cursor());
    auto it = r.begin();
    ++it; // skip B's entry

    blaster.run_direct(*it); // A's LDP
    CHECK(blaster.regs().DP.hi == da1.id.value);
    CHECK(blaster.regs().DP.lo == da1.offset);
    CHECK(blaster.regs().DS == 2);

    ++it;
    blaster.run_direct(*it); // A's INCDP
    CHECK(blaster.regs().DP.hi == da1.id.value); // hi unchanged
    CHECK(blaster.regs().DP.lo == da2.offset);    // skipped over B's data
    CHECK(blaster.regs().DS == 2);
  }
}
