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

TEST_CASE("Indirect buffer iteration", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto mgr = std::make_shared<pepp::bts::BufferManager>();
  using namespace tvm::EncodedOp;
  using M = tvm::RegMask;
  tvm::TraceBuffer tb(mgr, 1);
  constexpr u16 S = 0;

  auto body = [&](auto enc) { tb.emit_body(S, {enc.data(), enc.size()}); };

  // Submit N programs, each setting MOD1.lo to its index.
  constexpr int N = 5;
  auto before = tb.cursor();
  for (int i = 0; i < N; ++i) {
    tb.begin(S);
    body(LDMOD1Lo{static_cast<u16>(i)}.encode());
    tb.end(S);
  }
  auto after = tb.cursor();

  SECTION("Forward iteration visits all programs in submission order") {
    RegisterBlaster blaster(mgr);
    int count = 0;
    for (auto loc : tb.range(before, after)) {
      blaster.run_direct(loc);
      CHECK(blaster.regs().MOD1.lo == count);
      count++;
    }
    CHECK(count == N);
  }

  SECTION("Reverse iteration visits all programs in reverse order") {
    auto r = tb.range(before, after);
    RegisterBlaster blaster(mgr);
    int count = N;
    for (auto it = r.rbegin(); it != r.rend(); ++it) {
      count--;
      blaster.run_direct(*it);
      CHECK(blaster.regs().MOD1.lo == count);
    }
    CHECK(count == 0);
  }

  SECTION("Forward then backward round-trip") {
    auto r = tb.range(before, after);
    auto it = r.begin();

    // Walk forward to the third entry.
    RegisterBlaster blaster(mgr);
    for (int i = 0; i < 3; ++i)
      ++it;
    blaster.run_direct(*it);
    CHECK(blaster.regs().MOD1.lo == 3);

    // Walk backward two steps.
    --it;
    --it;
    blaster.run_direct(*it);
    CHECK(blaster.regs().MOD1.lo == 1);

    // Walk forward to the end, collecting remaining values.
    std::vector<u16> values;
    for (; it != r.end(); ++it) {
      blaster.run_direct(*it);
      values.push_back(blaster.regs().MOD1.lo);
    }
    CHECK(values == std::vector<u16>{1, 2, 3, 4});
  }

  SECTION("Sub-range iteration") {
    // Iterate only entries [1, 4) — should see programs 1, 2, 3.
    tvm::Cursor from{before.slot, static_cast<u16>(before.entry + 1)};
    tvm::Cursor to{before.slot, static_cast<u16>(before.entry + 4)};
    auto r = tb.range(from, to);

    RegisterBlaster blaster(mgr);
    std::vector<u16> values;
    for (auto loc : r) {
      blaster.run_direct(loc);
      values.push_back(blaster.regs().MOD1.lo);
    }
    CHECK(values == std::vector<u16>{1, 2, 3});
  }

  SECTION("Empty range produces no iterations") {
    auto r = tb.range(before, before);
    int count = 0;
    for ([[maybe_unused]] auto loc : r)
      count++;
    CHECK(count == 0);
  }
}

TEST_CASE("Cross-slot iteration", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto mgr = std::make_shared<pepp::bts::BufferManager>();
  using namespace tvm::EncodedOp;
  tvm::TraceBuffer tb(mgr, 1);
  constexpr u16 S = 0;
  constexpr u16 MAX = tvm::TraceBuffer::MAX_INDIRECT_ENTRIES;

  auto body = [&](auto enc) { tb.emit_body(S, {enc.data(), enc.size()}); };

  // Fill slot 0 with MAX-2 empty submissions, then 2 tagged ones at the tail.
  for (u16 i = 0; i < MAX - 2; ++i) {
    tb.begin(S);
    tb.end(S);
  }
  auto boundary_start = tb.cursor(); // {0, MAX-2}

  // Last 2 entries of slot 0.
  tb.begin(S);
  body(LDMOD1Lo{0xAA00}.encode());
  tb.end(S);
  tb.begin(S);
  body(LDMOD1Lo{0xAA01}.encode());
  tb.end(S);
  // Slot 0 is now full,  advance_slot fired, _head=1.

  // First 2 entries of slot 1.
  tb.begin(S);
  body(LDMOD1Lo{0xBB00}.encode());
  tb.end(S);
  tb.begin(S);
  body(LDMOD1Lo{0xBB01}.encode());
  tb.end(S);
  auto boundary_end = tb.cursor(); // {1, 2}

  SECTION("Forward iteration crosses slot boundary") {
    RegisterBlaster blaster(mgr);
    std::vector<u16> values;
    for (auto loc : tb.range(boundary_start, boundary_end)) {
      blaster.run_direct(loc);
      values.push_back(blaster.regs().MOD1.lo);
    }
    CHECK(values == std::vector<u16>{0xAA00, 0xAA01, 0xBB00, 0xBB01});
  }

  SECTION("Reverse iteration crosses slot boundary") {
    auto r = tb.range(boundary_start, boundary_end);
    RegisterBlaster blaster(mgr);
    std::vector<u16> values;
    for (auto it = r.rbegin(); it != r.rend(); ++it) {
      blaster.run_direct(*it);
      values.push_back(blaster.regs().MOD1.lo);
    }
    CHECK(values == std::vector<u16>{0xBB01, 0xBB00, 0xAA01, 0xAA00});
  }

  SECTION("Forward-backward round-trip across boundary") {
    auto r = tb.range(boundary_start, boundary_end);
    RegisterBlaster blaster(mgr);
    auto it = r.begin();

    // Forward past boundary into slot 1.
    ++it; // 0xAA01 (slot 0)
    ++it; // 0xBB00 (slot 1)
    blaster.run_direct(*it);
    CHECK(blaster.regs().MOD1.lo == 0xBB00);

    // Step back across boundary into slot 0.
    --it; // 0xAA01 (slot 0)
    blaster.run_direct(*it);
    CHECK(blaster.regs().MOD1.lo == 0xAA01);

    // Forward again to the end.
    ++it; // 0xBB00
    ++it; // 0xBB01
    blaster.run_direct(*it);
    CHECK(blaster.regs().MOD1.lo == 0xBB01);
  }
}
