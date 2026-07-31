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

TEST_CASE("run_indirect with iterator pair", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto mgr = std::make_shared<pepp::bts::BufferManager>();
  using namespace tvm::EncodedOp;
  using M = tvm::RegMask;
  tvm::TraceBuffer tb(mgr, 1);
  constexpr u16 S = 0;

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
    tb.end(S);
  }
  auto after = tb.cursor();

  SECTION("Forward iteration executes all programs") {
    auto r = tb.range(before, after);
    RegisterBlaster blaster(mgr);
    blaster.run_indirect(r.begin(), r.end());
    // Last program sets ACCESS = N-1.
    CHECK(blaster.regs().ACCESS == N - 1);
  }

  SECTION("Reverse iteration executes all programs in reverse") {
    auto r = tb.range(before, after);
    RegisterBlaster blaster(mgr);
    blaster.run_indirect(r.rbegin(), r.rend());
    // Last program executed sets ACCESS = 0 (the first submitted program).
    CHECK(blaster.regs().ACCESS == 0);
  }

  SECTION("Sub-range iteration executes only selected programs") {
    tvm::Cursor from{before.slot, static_cast<u16>(before.entry + 1)};
    tvm::Cursor to{before.slot, static_cast<u16>(before.entry + 4)};
    auto r = tb.range(from, to);

    RegisterBlaster blaster(mgr);
    blaster.run_indirect(r.begin(), r.end());
    // Programs 1, 2, 3 executed; last one sets ACCESS = 3.
    CHECK(blaster.regs().ACCESS == 3);
  }

  SECTION("Empty range is a no-op") {
    auto r = tb.range(before, before);
    RegisterBlaster blaster(mgr);
    blaster.run_indirect(r.begin(), r.end());
    // ACCESS should remain at its default (0).
    CHECK(blaster.regs().ACCESS == 0);
  }

  SECTION("Hard stop aborts iteration early") {
    // Submit 3 more programs: 0xAA is normal, 0xBB triggers a hard stop (CLRMEM without a system), 0xCC is normal.
    auto mid = tb.cursor();
    tb.begin(S);
    set_access(0xAA);
    tb.end(S);

    tb.begin(S);
    set_access(0xBB);
    // CLRMEM without a system causes hard_stop(MissingSystem).
    auto clr = ClrMem<1>{0}.encode();
    tb.emit_postfix(S, {clr.data(), clr.size()});
    tb.end(S);

    tb.begin(S);
    set_access(0xCC);
    tb.end(S);

    auto end_cursor = tb.cursor();
    auto r = tb.range(mid, end_cursor);
    RegisterBlaster blaster(mgr);
    blaster.run_indirect(r.begin(), r.end());

    // Program 0xAA executed normally, then 0xBB set ACCESS but CLRMEM hard-stopped, so 0xCC was never reached.
    CHECK(blaster.regs().ACCESS == 0xBB);
    CHECK(blaster.csrs().F == 1);
  }
}
