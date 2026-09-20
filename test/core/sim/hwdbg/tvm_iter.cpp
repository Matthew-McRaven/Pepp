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
#include <algorithm>
#include <array>
#include <catch.hpp>
#include <stdexcept>
#include <vector>

#include "core/math/bitmanip/leb128.hpp"
#include "core/sim/debugger/tvm_apply_backend.hpp"
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
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
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
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
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
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
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
    tvm::Cursor from{before.slot, static_cast<u16>(before.ordinal + 1)};
    tvm::Cursor to{before.slot, static_cast<u16>(before.ordinal + 4)};
    auto r = tb.range(from, to);

    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
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
  auto body = [&](auto enc) { tb.emit_body(S, {enc.data(), enc.size()}); };

  // Fill slot 0, tagging every submission, until the commit that fills it advances the ring. How many entries a slot
  // holds depends on how well they encoded, so the last two are found by filling rather than by counting.
  tvm::Cursor boundary_start{}, last_cursor{};
  u16 tag = 0, second_last_tag = 0, last_tag = 0;
  while (tb.cursor().slot == 0) {
    boundary_start = last_cursor;
    last_cursor = tb.cursor();
    second_last_tag = last_tag;
    last_tag = (u16)(0xAA00 + tag++);
    tb.begin(S);
    body(LDMOD1Lo{last_tag}.encode());
    tb.commit(S);
  }
  // boundary_start now names the second-to-last entry of slot 0, and advance_slot has fired, so _head=1.

  // First 2 entries of slot 1.
  tb.begin(S);
  body(LDMOD1Lo{0xBB00}.encode());
  tb.commit(S);
  tb.begin(S);
  body(LDMOD1Lo{0xBB01}.encode());
  tb.commit(S);
  auto boundary_end = tb.cursor(); // {1, 2}

  SECTION("Forward iteration crosses slot boundary") {
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    std::vector<u16> values;
    for (auto loc : tb.range(boundary_start, boundary_end)) {
      blaster.run(loc);
      values.push_back(blaster.regs().MOD1.lo);
    }
    CHECK(values == std::vector<u16>{second_last_tag, last_tag, 0xBB00, 0xBB01});
  }

  SECTION("Reverse iteration crosses slot boundary") {
    auto r = tb.range(boundary_start, boundary_end);
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    std::vector<u16> values;
    for (auto it = r.rbegin(); it != r.rend(); ++it) {
      blaster.run(*it);
      values.push_back(blaster.regs().MOD1.lo);
    }
    CHECK(values == std::vector<u16>{0xBB01, 0xBB00, last_tag, second_last_tag});
  }

  SECTION("Forward-backward round-trip across boundary") {
    auto r = tb.range(boundary_start, boundary_end);
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    auto it = r.begin();

    // Forward past boundary into slot 1.
    ++it; // last of slot 0
    ++it; // 0xBB00 (slot 1)
    blaster.run(*it);
    CHECK(blaster.regs().MOD1.lo == 0xBB00);

    // Step back across boundary into slot 0.
    --it; // last of slot 0
    blaster.run(*it);
    CHECK(blaster.regs().MOD1.lo == last_tag);

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
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    auto stopped_at = blaster.run_each(r.begin(), r.end());
    // Last program sets ACCESS = N-1.
    CHECK(blaster.regs().ACCESS == N - 1);
    // Nothing failed, so the walk consumed the whole range.
    CHECK(stopped_at == r.end());
  }

  SECTION("The span overload reports how many programs ran") {
    auto r = tb.range(before, after);
    std::vector<tvm::ProgramLocation> locs(r.begin(), r.end());
    REQUIRE(locs.size() == N);

    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    CHECK(blaster.run_each(locs) == N);
    CHECK(blaster.regs().ACCESS == N - 1);
  }

  SECTION("Reverse iteration executes all programs in reverse") {
    auto r = tb.range(before, after);
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    blaster.run_each(r.rbegin(), r.rend());
    // Last program executed sets ACCESS = 0 (the first submitted program).
    CHECK(blaster.regs().ACCESS == 0);
  }

  SECTION("Sub-range iteration executes only selected programs") {
    tvm::Cursor from{before.slot, static_cast<u16>(before.ordinal + 1)};
    tvm::Cursor to{before.slot, static_cast<u16>(before.ordinal + 4)};
    auto r = tb.range(from, to);

    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    blaster.run_each(r.begin(), r.end());
    // Programs 1, 2, 3 executed; last one sets ACCESS = 3.
    CHECK(blaster.regs().ACCESS == 3);
  }

  SECTION("Empty range is a no-op") {
    auto r = tb.range(before, before);
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
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
    auto clr = ClrMem<2>{.dev = 0, .reset = 0}.encode();
    tb.emit_postfix(S, {clr.data(), clr.size()});
    tb.commit(S);

    tb.begin(S);
    set_access(0xCC);
    tb.commit(S);

    auto end_cursor = tb.cursor();
    auto r = tb.range(mid, end_cursor);
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    auto stopped_at = blaster.run_each(r.begin(), r.end());

    // Program 0xAA executed normally, then 0xBB set ACCESS but CLRMEM hard-stopped, so 0xCC was never reached.
    CHECK(blaster.regs().ACCESS == 0xBB);
    CHECK(blaster.csrs().F == 1);
    CHECK(blaster.stop_cause() == StopCause::MissingSystem);

    // The return value is the whole reason a caller can say *which* program failed rather than just that one did.
    // It points at the offender, not past it: the second of the three.
    auto expected = r.begin();
    ++expected;
    CHECK(stopped_at == expected);

    // Same story through the span overload, which counts programs run -- the failing one included.
    std::vector<tvm::ProgramLocation> locs(r.begin(), r.end());
    REQUIRE(locs.size() == 3);
    tvm::Interpreter counted(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    CHECK(counted.run_each(locs) == 2);
  }
}

TEST_CASE("tvm::TraceBuffer: location entry codec", "[scope:core][scope:core.dbg][kind:unit][arch:*][!throws]") {
  using L = pepp::bts::Buffer::Location;
  auto loc = [](u16 id, u16 off) { return L{pepp::bts::Buffer::ID{id}, off}; };
  auto program = [&](u16 cid, u16 coff, u16 did, u16 doff) {
    return tvm::ProgramLocation{loc(cid, coff), loc(did, doff)};
  };
  std::array<u8, tvm::TraceBuffer::MAX_LOCATION_ENTRY_BYTES> buf{};

  // Every pair here round-trips against the same anchor, and each says what it costs to encode.
  const auto anchor = program(4, 0x1000, 7, 0x2000);
  struct Case {
    const char *why;
    tvm::ProgramLocation value;
    u8 bytes;
  };
  const Case cases[] = {
      {"the anchor itself is two zero deltas", anchor, 2},
      {"a program a few bytes along", program(4, 0x1006, 7, 0x2004), 2},
      {"data that moved backwards costs no more than forwards", program(4, 0x1006, 7, 0x1FF0), 2},
      {"a record that wrote no payload", program(4, 0x1006, 0, 0), 4},
      {"both chains rolled to another buffer", program(9, 0x0004, 3, 0x0010), 6},
  };
  for (const auto &c : cases) {
    INFO(c.why);
    const u8 wrote = tvm::TraceBuffer::encode_location(buf, anchor, c.value);
    CHECK(wrote == c.bytes);
    u8 read = 0;
    const auto back = tvm::TraceBuffer::decode_location({buf.data(), wrote}, anchor, read);
    CHECK(read == wrote);
    CHECK(back.code.id == c.value.code.id);
    CHECK(back.code.offset == c.value.code.offset);
    CHECK(back.data.id == c.value.data.id);
    CHECK(back.data.offset == c.value.data.offset);
  }

  SECTION("The size helper agrees with the encoder") {
    // emit_location() sizes an entry to decide whether it still fits in the group, then encodes it. The two have to
    // agree exactly, or it accounts for one length and writes another.
    for (const auto &c : cases) {
      INFO(c.why);
      const auto code = (i64)c.value.code.as_u32() - (i64)anchor.code.as_u32();
      const auto data = (i64)c.value.data.as_u32() - (i64)anchor.data.as_u32();
      CHECK(bits::getSLEB128Size(code) + bits::getSLEB128Size(data) ==
            tvm::TraceBuffer::encode_location(buf, anchor, c.value));
    }
  }

  SECTION("An entry never exceeds its worst case") {
    // The widest possible pair: both fields jump the full 32-bit range. Nothing bounds-checks the encoder, so this
    // is what says a MAX_LOCATION_ENTRY_BYTES buffer is always enough for it.
    const auto far = program(0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF);
    CHECK(tvm::TraceBuffer::encode_location(buf, tvm::ProgramLocation{}, far) <=
          tvm::TraceBuffer::MAX_LOCATION_ENTRY_BYTES);
  }

  SECTION("A buffer too small for the entry is refused") {
    // An entry near the end of a group has only the bytes left in that group, so what matters is whether this pair
    // fits, not whether a worst-case one would.
    const auto value = program(9, 0x0004, 3, 0x0010);
    std::array<u8, tvm::TraceBuffer::MAX_LOCATION_ENTRY_BYTES> narrow{};
    const u8 needs = tvm::TraceBuffer::encode_location(narrow, anchor, value);
    REQUIRE(needs > 2);
    narrow.fill(0);
    CHECK_THROWS_AS(tvm::TraceBuffer::encode_location({narrow.data(), std::size_t(needs - 1)}, anchor, value),
                    std::out_of_range);
    CHECK(std::ranges::all_of(narrow, [](u8 b) { return b == 0; }));
    // A buffer that fits this pair is enough, even though a worst-case entry would not fit in it.
    CHECK(tvm::TraceBuffer::encode_location({narrow.data(), std::size_t(needs)}, anchor, value) == needs);
  }

  SECTION("A truncated entry decodes as nothing rather than reading past its span") {
    const auto value = program(9, 0x0004, 3, 0x0010);
    const u8 wrote = tvm::TraceBuffer::encode_location(buf, anchor, value);
    for (u8 len = 0; len < wrote; ++len) {
      INFO("truncated to " << (int)len << " of " << (int)wrote);
      u8 read = 0xFF;
      tvm::TraceBuffer::decode_location({buf.data(), len}, anchor, read);
      CHECK(read == 0);
    }
  }
}
