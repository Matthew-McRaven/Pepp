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
#include <utility>

#include "core/sim/debugger/tvm_tracebuffer.hpp"

namespace {

// A one-slot ring laps after a single slot's worth of entries, which makes the wrap cheap to reach.
constexpr u16 ENTRIES_PER_SLOT = tvm::TraceBuffer::MAX_LOCATION_ENTRIES;

// Submit `count` empty programs -- end() still appends a HALT -- and report the first and last locations.
std::pair<tvm::ProgramLocation, tvm::ProgramLocation> submit_empty(tvm::TraceBuffer &tb, size_t count) {
  constexpr Device::ID S{1};
  tvm::ProgramLocation first{}, last{};
  for (size_t i = 0; i < count; ++i) {
    tb.begin(S);
    last = tb.commit(S);
    if (i == 0) first = last;
  }
  return {first, last};
}

bool same_location(tvm::ProgramLocation a, tvm::ProgramLocation b) {
  return a.code.id.value == b.code.id.value && a.code.offset == b.code.offset;
}

} // namespace

TEST_CASE("tvm::Interpreter: Watermark callbacks", "[scope:core][scope:core.dbg][kind:unit][arch:pep10][!throws]") {
  auto mgr = std::make_shared<pepp::bts::BufferManager>();
  constexpr Device::ID S{1};

  // Fill the current slot completely, triggering advance_slot.
  auto fill_slot = [&](tvm::TraceBuffer &tb) {
    for (u16 i = 0; i < tvm::TraceBuffer::MAX_LOCATION_ENTRIES; ++i) {
      tb.begin(S);
      tb.commit(S);
    }
  };

  // Filling a slot never throws on its own. The slot advances at the end of the commit that filled it -- firing
  // watermarks and taking occupancy up -- and the refusal happens on the way *into* the next instruction, which is
  // this. Splitting them is what keeps RingOverflow out of commit(), and therefore out of a destructor unwinding
  // from a faulted instruction.
  auto one_more = [&](tvm::TraceBuffer &tb) {
    tb.begin(S);
    tb.commit(S);
  };

  SECTION("Half-watermark fires on ping-pong ring") {
    tvm::TraceBuffer tb(mgr, 2);
    int fires = 0;
    tb.on_watermark(0.5f, [&]() { fires++; });

    CHECK(tb.ring_occupancy() == Catch::Approx(0.0f));
    fill_slot(tb);
    CHECK(fires == 1);
    CHECK(tb.ring_occupancy() == Catch::Approx(0.5f));
  }

  SECTION("Multiple watermarks fire at distinct thresholds") {
    tvm::TraceBuffer tb(mgr, 2);
    int half_fires = 0, full_fires = 0;
    tb.on_watermark(0.5f, [&]() { half_fires++; });
    tb.on_watermark(1.0f, [&]() { full_fires++; });

    fill_slot(tb);
    CHECK(half_fires == 1);
    CHECK(full_fires == 0);
    CHECK(tb.ring_occupancy() == Catch::Approx(0.5f));

    // Filling the second slot takes the ring to capacity. The 1.0 watermark fires as the last chance to drain, and
    // since this callback doesn't, the next instruction has nowhere to go.
    fill_slot(tb);
    CHECK(half_fires == 1);
    CHECK(full_fires == 1);
    CHECK(tb.ring_occupancy() == Catch::Approx(1.0f));
    CHECK_THROWS_AS(one_more(tb), tvm::RingOverflow);
  }

  SECTION("Watermark does not re-fire without downward crossing") {
    tvm::TraceBuffer tb(mgr, 2);
    int fires = 0;
    tb.on_watermark(0.5f, [&]() { fires++; });

    fill_slot(tb); // occ=0.5, fires
    CHECK(fires == 1);

    // occ=1.0, still above 0.5 so no re-fire. Reaching capacity undrained also blocks the instruction after it.
    fill_slot(tb);
    CHECK(fires == 1);
    CHECK_THROWS_AS(one_more(tb), tvm::RingOverflow);
  }

  SECTION("Acknowledge resets watermark, allowing re-fire") {
    tvm::TraceBuffer tb(mgr, 2);
    int fires = 0;
    tb.on_watermark(0.5f, [&]() { fires++; });

    fill_slot(tb); // _head=1, occ=0.5, fires
    CHECK(fires == 1);

    tb.acknowledge({1, 0}); // _tail=1, occ=0.0, watermark reset
    CHECK(tb.ring_occupancy() == Catch::Approx(0.0f));

    fill_slot(tb); // _head=2, occ=0.5, fires again
    CHECK(fires == 2);
  }

  SECTION("Acknowledge at threshold boundary does not reset watermark") {
    tvm::TraceBuffer tb(mgr, 2);
    int fires = 0;
    tb.on_watermark(0.5f, [&]() { fires++; });

    fill_slot(tb); // occ=0.5, fires
    fill_slot(tb); // occ=1.0, ring is now full and undrained
    CHECK(fires == 1);
    CHECK_THROWS_AS(one_more(tb), tvm::RingOverflow);

    // Acknowledge one slot so occ drops to 0.5, exactly at threshold.
    // Strict < means fired flag is NOT reset.
    tb.acknowledge({1, 0});
    CHECK(tb.ring_occupancy() == Catch::Approx(0.5f));

    // occ=1.0, watermark still armed so no re-fire -- and full again, so the instruction after refuses again.
    fill_slot(tb);
    CHECK(fires == 1);
    CHECK_THROWS_AS(one_more(tb), tvm::RingOverflow);

    // Acknowledge both consumed slots so occ drops to 0.0, which is below threshold and therefore resets.
    tb.acknowledge({3, 0});
    CHECK(tb.ring_occupancy() == Catch::Approx(0.0f));

    fill_slot(tb); // occ=0.5, fires
    CHECK(fires == 2);
  }
}

TEST_CASE("tvm::Interpreter: Throw rather than overwrite old data",
          "[scope:core][scope:core.dbg][kind:unit][arch:pep10][!throws]") {
  auto mgr = std::make_shared<pepp::bts::BufferManager>();
  tvm::TraceBuffer tb(mgr, 1);
  constexpr Device::ID S{1};

  tb.begin(S);
  auto first = tb.commit(S);

  // Filling the rest of the single slot leaves the ring nowhere to advance to, because nothing has been
  // acknowledged. Rather than lapping onto trace no one has read, the instruction after the last one that fit
  // refuses -- so this asks for one more than the slot holds, and the final begin() is the one that throws.
  CHECK_THROWS_AS(submit_empty(tb, (size_t)ENTRIES_PER_SLOT), tvm::RingOverflow);

  // Everything accepted before the refusal is intact -- in particular the oldest entry, which a lap would clobber.
  auto entry0 = *tb.range(tvm::Cursor{.slot = 0, .entry = 0}, tb.cursor()).begin();
  CHECK(same_location(entry0, first));
  CHECK(tb.instruction_count() == (size_t)ENTRIES_PER_SLOT);
}

TEST_CASE("tvm::Interpreter: Resume submission after overflow", "[scope:core][scope:core.dbg][kind:unit][arch:pep10][!throws]") {
  auto mgr = std::make_shared<pepp::bts::BufferManager>();
  tvm::TraceBuffer tb(mgr, 1);
  constexpr Device::ID S{1};

  // One past what the single slot holds: the last begin() finds the ring lapped and refuses.
  REQUIRE_THROWS_AS(submit_empty(tb, (size_t)ENTRIES_PER_SLOT + 1), tvm::RingOverflow);

  // Consuming the slot is what unblocks the ring: acknowledge() resets it, and the next submission lands in a clean
  // slot rather than on top of the old trace.
  tb.acknowledge({1, 0});
  CHECK(tb.ring_occupancy() == Catch::Approx(0.0f));

  tb.begin(S);
  CHECK_NOTHROW(tb.commit(S));
}

TEST_CASE("tvm::Interpreter: Just-in-time emptying of ring", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto mgr = std::make_shared<pepp::bts::BufferManager>();
  tvm::TraceBuffer tb(mgr, 1);

  // Watermark callbacks run before the overflow check precisely so a callback like this one can keep the ring
  // moving. Draining here means the advance finds a free slot and never throws. A real consumer would iterate the
  // pending range first -- acknowledge() frees the slot's chains, so any Location handed out for it dies here.
  tb.on_watermark(1.0f, [&]() { tb.acknowledge(tb.cursor()); });

  CHECK_NOTHROW(submit_empty(tb, (size_t)ENTRIES_PER_SLOT));
  CHECK(tb.instruction_count() == (size_t)ENTRIES_PER_SLOT);
}

TEST_CASE("tvm::TraceBuffer: clear()", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto mgr = std::make_shared<pepp::bts::BufferManager>();
  constexpr Device::ID S{1};
  tvm::TraceBuffer tb(mgr, 2);
  tb.trace(S);
  tb.set_address_in_payload(S, true);

  // Record something before dropping it.
  auto record_and_clear = [&]() {
    submit_empty(tb, 16);
    REQUIRE(tb.instruction_count() == 16);
    REQUIRE(tb.footprint().code > 0);
    REQUIRE(tb.cursor() != tvm::Cursor{});
    tb.clear();
  };

  SECTION("drops recorded traces") {
    record_and_clear();
    CHECK(tb.instruction_count() == 0);
    CHECK(tb.footprint().programs == 0);
    CHECK(tb.footprint().code == 0);
    CHECK(tb.footprint().stencils == 0);
    CHECK(tb.cursor() == tvm::Cursor{});
    CHECK(tb.ring_occupancy() == Catch::Approx(0.0f));
  }

  SECTION("preserves configuration values") {
    record_and_clear();
    CHECK(tb.traced(S));
    CHECK(tb.address_in_payload(S));
  }

  SECTION("allows the buffer to be re-used") {
    record_and_clear();
    // Exercises the re-seeded tombstone: begin() writes it into the entry it reserves, so a stencil chain cleared
    // without re-appending it would leave that entry aimed at a freed program.
    submit_empty(tb, 4);
    CHECK(tb.instruction_count() == 4);
    CHECK(tb.footprint().code > 0);
  }

  SECTION("aborts an open recording rather than refusing") {
    // Node::reset asserts open == 0 to avoid a use-after-free on data. A caller resetting the machine should not have
    // to close recordings first.
    tb.begin(S);
    REQUIRE(tb.is_recording(S));

    CHECK_NOTHROW(tb.clear());
    CHECK_FALSE(tb.is_recording(S));

    // The reservation was released rather than leaked, so the slot is free to record into again.
    tb.begin(S);
    CHECK_NOTHROW(tb.commit(S));
    CHECK(tb.instruction_count() == 1);
  }

  SECTION("keeps watermark registrations and re-arms them") {
    int fires = 0;
    tb.on_watermark(0.5f, [&]() { fires++; });
    submit_empty(tb, tvm::TraceBuffer::MAX_LOCATION_ENTRIES);
    REQUIRE(fires == 1);

    tb.clear();
    // The callback belongs to the UI and outlives any one run but should be re-armed so it can fire again.
    CHECK(tb.ring_occupancy() == Catch::Approx(0.0f));
    submit_empty(tb, tvm::TraceBuffer::MAX_LOCATION_ENTRIES);
    CHECK(fires == 2);
  }
}
