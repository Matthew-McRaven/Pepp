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

#include "core/sim/debugger/tvm_tracebuffer.hpp"

TEST_CASE("Watermark callbacks", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto mgr = std::make_shared<pepp::bts::BufferManager>();
  constexpr u16 S = 0;

  // Fill the current slot completely, triggering advance_slot.
  auto fill_slot = [&](tvm::TraceBuffer &tb) {
    for (u16 i = 0; i < tvm::TraceBuffer::MAX_INDIRECT_ENTRIES; ++i) {
      tb.begin(S);
      tb.end(S);
    }
  };

  SECTION("Half-watermark fires on ping-pong ring") {
    tvm::TraceBuffer tb(mgr, 1, 2);
    int fires = 0;
    tb.on_watermark(0.5f, [&]() { fires++; });

    CHECK(tb.ring_occupancy() == Catch::Approx(0.0f));
    fill_slot(tb);
    CHECK(fires == 1);
    CHECK(tb.ring_occupancy() == Catch::Approx(0.5f));
  }

  SECTION("Multiple watermarks fire at distinct thresholds") {
    tvm::TraceBuffer tb(mgr, 1, 2);
    int half_fires = 0, full_fires = 0;
    tb.on_watermark(0.5f, [&]() { half_fires++; });
    tb.on_watermark(1.0f, [&]() { full_fires++; });

    fill_slot(tb);
    CHECK(half_fires == 1);
    CHECK(full_fires == 0);
    CHECK(tb.ring_occupancy() == Catch::Approx(0.5f));

    fill_slot(tb);
    CHECK(half_fires == 1);
    CHECK(full_fires == 1);
    CHECK(tb.ring_occupancy() == Catch::Approx(1.0f));
  }

  SECTION("Watermark does not re-fire without downward crossing") {
    tvm::TraceBuffer tb(mgr, 1, 2);
    int fires = 0;
    tb.on_watermark(0.5f, [&]() { fires++; });

    fill_slot(tb); // occ=0.5, fires
    CHECK(fires == 1);

    fill_slot(tb); // occ=1.0, still above 0.5 so no re-fire
    CHECK(fires == 1);
  }

  SECTION("Acknowledge resets watermark, allowing re-fire") {
    tvm::TraceBuffer tb(mgr, 1, 2);
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
    tvm::TraceBuffer tb(mgr, 1, 2);
    int fires = 0;
    tb.on_watermark(0.5f, [&]() { fires++; });

    fill_slot(tb); // occ=0.5, fires
    fill_slot(tb); // occ=1.0
    CHECK(fires == 1);

    // Acknowledge one slot so occ drops to 0.5, exactly at threshold.
    // Strict < means fired flag is NOT reset.
    tb.acknowledge({1, 0});
    CHECK(tb.ring_occupancy() == Catch::Approx(0.5f));

    fill_slot(tb); // occ=1.0, watermark still armed so no re-fire
    CHECK(fires == 1);

    // Acknowledge both consumed slots so occ drops to 0.0, which is below threshold and therefore resets.
    tb.acknowledge({3, 0});
    CHECK(tb.ring_occupancy() == Catch::Approx(0.0f));

    fill_slot(tb); // occ=0.5, fires
    CHECK(fires == 2);
  }
}
