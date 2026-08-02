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

TEST_CASE("tvm::Interpreter: Location buffer iteration", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto mgr = std::make_shared<pepp::bts::BufferManager>();
  using namespace tvm::EncodedOp;
  using M = tvm::RegMask;
  tvm::TraceBuffer tb(mgr);
  constexpr Device::ID S{1};

  auto body = [&](auto enc) { tb.emit_body(S, {enc.data(), enc.size()}); };

  // Submit N programs, each setting MOD1.lo to its index.
  constexpr int N = 5;
  auto before = tb.cursor();
  for (int i = 0; i < N; ++i) {
    tb.begin(S);
    body(LDMOD1Lo{static_cast<u16>(i)}.encode());
    tb.commit(S);
  }
  auto after = tb.cursor();

  SECTION("Forward iteration visits all programs in submission order") {
    tvm::Interpreter blaster(mgr);
    int count = 0;
    for (auto loc : tb.range(before, after)) {
      blaster.run(loc);
      CHECK(blaster.regs().MOD1.lo == count);
      count++;
    }
    CHECK(count == N);
  }

  SECTION("Reverse iteration visits all programs in reverse order") {
    auto r = tb.range(before, after);
    tvm::Interpreter blaster(mgr);
    int count = N;
    for (auto it = r.rbegin(); it != r.rend(); ++it) {
      count--;
      blaster.run(*it);
      CHECK(blaster.regs().MOD1.lo == count);
    }
    CHECK(count == 0);
  }

  SECTION("Forward then backward round-trip") {
    auto r = tb.range(before, after);
    auto it = r.begin();

    // Walk forward to the third entry.
    tvm::Interpreter blaster(mgr);
    for (int i = 0; i < 3; ++i)
      ++it;
    blaster.run(*it);
    CHECK(blaster.regs().MOD1.lo == 3);

    // Walk backward two steps.
    --it;
    --it;
    blaster.run(*it);
    CHECK(blaster.regs().MOD1.lo == 1);

    // Walk forward to the end, collecting remaining values.
    std::vector<u16> values;
    for (; it != r.end(); ++it) {
      blaster.run(*it);
      values.push_back(blaster.regs().MOD1.lo);
    }
    CHECK(values == std::vector<u16>{1, 2, 3, 4});
  }

  SECTION("Sub-range iteration") {
    // Iterate only entries [1, 4) — should see programs 1, 2, 3.
    tvm::Cursor from{before.slot, static_cast<u16>(before.entry + 1)};
    tvm::Cursor to{before.slot, static_cast<u16>(before.entry + 4)};
    auto r = tb.range(from, to);

    tvm::Interpreter blaster(mgr);
    std::vector<u16> values;
    for (auto loc : r) {
      blaster.run(loc);
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
  tvm::TraceBuffer tb(mgr);
  constexpr Device::ID S{1};
  constexpr u16 MAX = tvm::TraceBuffer::MAX_LOCATION_ENTRIES;

  auto body = [&](auto enc) { tb.emit_body(S, {enc.data(), enc.size()}); };

  // Fill slot 0 with MAX-2 empty submissions, then 2 tagged ones at the tail.
  for (u16 i = 0; i < MAX - 2; ++i) {
    tb.begin(S);
    tb.commit(S);
  }
  auto boundary_start = tb.cursor(); // {0, MAX-2}

  // Last 2 entries of slot 0.
  tb.begin(S);
  body(LDMOD1Lo{0xAA00}.encode());
  tb.commit(S);
  tb.begin(S);
  body(LDMOD1Lo{0xAA01}.encode());
  tb.commit(S);
  // Slot 0 is now full,  advance_slot fired, _head=1.

  // First 2 entries of slot 1.
  tb.begin(S);
  body(LDMOD1Lo{0xBB00}.encode());
  tb.commit(S);
  tb.begin(S);
  body(LDMOD1Lo{0xBB01}.encode());
  tb.commit(S);
  auto boundary_end = tb.cursor(); // {1, 2}

  SECTION("Forward iteration crosses slot boundary") {
    tvm::Interpreter blaster(mgr);
    std::vector<u16> values;
    for (auto loc : tb.range(boundary_start, boundary_end)) {
      blaster.run(loc);
      values.push_back(blaster.regs().MOD1.lo);
    }
    CHECK(values == std::vector<u16>{0xAA00, 0xAA01, 0xBB00, 0xBB01});
  }

  SECTION("Reverse iteration crosses slot boundary") {
    auto r = tb.range(boundary_start, boundary_end);
    tvm::Interpreter blaster(mgr);
    std::vector<u16> values;
    for (auto it = r.rbegin(); it != r.rend(); ++it) {
      blaster.run(*it);
      values.push_back(blaster.regs().MOD1.lo);
    }
    CHECK(values == std::vector<u16>{0xBB01, 0xBB00, 0xAA01, 0xAA00});
  }

  SECTION("Forward-backward round-trip across boundary") {
    auto r = tb.range(boundary_start, boundary_end);
    tvm::Interpreter blaster(mgr);
    auto it = r.begin();

    // Forward past boundary into slot 1.
    ++it; // 0xAA01 (slot 0)
    ++it; // 0xBB00 (slot 1)
    blaster.run(*it);
    CHECK(blaster.regs().MOD1.lo == 0xBB00);

    // Step back across boundary into slot 0.
    --it; // 0xAA01 (slot 0)
    blaster.run(*it);
    CHECK(blaster.regs().MOD1.lo == 0xAA01);

    // Forward again to the end.
    ++it; // 0xBB00
    ++it; // 0xBB01
    blaster.run(*it);
    CHECK(blaster.regs().MOD1.lo == 0xBB01);
  }
}

TEST_CASE("tvm::Interpreter:  run_each with iterator pair", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto mgr = std::make_shared<pepp::bts::BufferManager>();
  using namespace tvm::EncodedOp;
  using M = tvm::RegMask;
  tvm::TraceBuffer tb(mgr);
  constexpr Device::ID S{1};

  // Use ACCESS (non-MOD, unaffected by CLRMOD) with clrmod=false to avoid
  // MOD clearing at the start of the HALT that end() appends.
  auto set_access = [&](u16 val) {
    auto enc = LMR_of<false>(std::pair{M::ACCESS, val});
    tb.emit_body(S, {enc.data(), enc.size()});
  };

  // Submit N programs, each setting ACCESS to its index.
  constexpr int N = 5;
  auto before = tb.cursor();
  for (int i = 0; i < N; ++i) {
    tb.begin(S);
    set_access(static_cast<u16>(i));
    tb.commit(S);
  }
  auto after = tb.cursor();

  SECTION("Forward iteration executes all programs") {
    auto r = tb.range(before, after);
    tvm::Interpreter blaster(mgr);
    blaster.run_each(r.begin(), r.end());
    // Last program sets ACCESS = N-1.
    CHECK(blaster.regs().ACCESS == N - 1);
  }

  SECTION("Reverse iteration executes all programs in reverse") {
    auto r = tb.range(before, after);
    tvm::Interpreter blaster(mgr);
    blaster.run_each(r.rbegin(), r.rend());
    // Last program executed sets ACCESS = 0 (the first submitted program).
    CHECK(blaster.regs().ACCESS == 0);
  }

  SECTION("Sub-range iteration executes only selected programs") {
    tvm::Cursor from{before.slot, static_cast<u16>(before.entry + 1)};
    tvm::Cursor to{before.slot, static_cast<u16>(before.entry + 4)};
    auto r = tb.range(from, to);

    tvm::Interpreter blaster(mgr);
    blaster.run_each(r.begin(), r.end());
    // Programs 1, 2, 3 executed; last one sets ACCESS = 3.
    CHECK(blaster.regs().ACCESS == 3);
  }

  SECTION("Empty range is a no-op") {
    auto r = tb.range(before, before);
    tvm::Interpreter blaster(mgr);
    blaster.run_each(r.begin(), r.end());
    // ACCESS should remain at its default (0).
    CHECK(blaster.regs().ACCESS == 0);
  }

  SECTION("Hard stop aborts iteration early") {
    // Submit 3 more programs: 0xAA is normal, 0xBB triggers a hard stop (CLRMEM without a system), 0xCC is normal.
    auto mid = tb.cursor();
    tb.begin(S);
    set_access(0xAA);
    tb.commit(S);

    tb.begin(S);
    set_access(0xBB);
    // CLRMEM without a system causes hard_stop(MissingSystem).
    auto clr = ClrMem<1>{0}.encode();
    tb.emit_postfix(S, {clr.data(), clr.size()});
    tb.commit(S);

    tb.begin(S);
    set_access(0xCC);
    tb.commit(S);

    auto end_cursor = tb.cursor();
    auto r = tb.range(mid, end_cursor);
    tvm::Interpreter blaster(mgr);
    blaster.run_each(r.begin(), r.end());

    // Program 0xAA executed normally, then 0xBB set ACCESS but CLRMEM hard-stopped, so 0xCC was never reached.
    CHECK(blaster.regs().ACCESS == 0xBB);
    CHECK(blaster.csrs().F == 1);
  }
}
