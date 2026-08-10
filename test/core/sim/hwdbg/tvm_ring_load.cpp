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
#include <memory>

#include "core/sim/debugger/trace_device.hpp"
#include "core/sim/debugger/trace_recorder.hpp"
#include "core/sim/debugger/tvm_apply_backend.hpp"
#include "core/sim/debugger/tvm_interpreter.hpp"
#include "core/sim/debugger/tvm_tracebuffer.hpp"
#include "core/sim/memory/ram/dense.hpp"
#include "core/sim/system.hpp"

// The ring is the part of this design that the other suites never actually drive. tvm_capacity covers the mechanics
// in isolation -- watermarks fire, overflow throws, a slot can be resumed -- but nothing there wraps the ring
// repeatedly while recording, and nothing reclaims a slot and then keeps going. That combination is where buffer
// recycling, generation-tagged ids, and lazily acquired location buffers all have to hold together, so it is where
// the remaining bugs would live.

namespace {
constexpr Device::ID S{1};
constexpr u16 PER_SLOT = tvm::TraceBuffer::MAX_LOCATION_ENTRIES;

// One record carrying a small payload, so the data chains are exercised alongside the code chains.
tvm::ProgramLocation record_one(tvm::TraceBuffer &tb, u16 tag) {
  tb.begin(S);
  const std::array<u8, 4> payload{u8(tag), u8(tag >> 8), 0xA5, 0x5A};
  tb.append_data(S, {payload.data(), payload.size()});
  const auto set = tvm::EncodedOp::LDR<tvm::RegMask::DS>{4}.encode();
  tb.emit_body(S, {set.data(), set.size()});
  return tb.commit(S);
}

// Release every slot before the one currently being written, so at most one slot is ever outstanding. That keeps the
// ring turning indefinitely, which is the steady state a live debugger session would run in.
void drain_behind(tvm::TraceBuffer &tb) {
  const auto c = tb.cursor();
  if (c.slot > 0) tb.acknowledge({c.slot, 0});
}
} // namespace

TEST_CASE("tvm::TraceBuffer: ring under sustained load", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto mgr = std::make_shared<pepp::bts::BufferManager>();

  SECTION("Wrapping the ring repeatedly neither throws nor leaks buffers") {
    tvm::TraceBuffer tb(mgr, 2);
    // Two full passes over a two-slot ring, so every slot is reclaimed and reused more than once. Each pass costs
    // MAX_LOCATION_ENTRIES commits, so this is deliberately not larger than it needs to be to prove the point.
    const std::size_t total = std::size_t(PER_SLOT) * 4 + 17;

    u16 peak_allocated = 0;
    for (std::size_t i = 0; i < total; ++i) {
      record_one(tb, u16(i));
      drain_behind(tb);
      peak_allocated = std::max(peak_allocated, mgr->allocated_buffers());
    }

    CHECK(tb.instruction_count() == total);
    // The whole point of the ring: live buffers are bounded by what is outstanding, not by how much was recorded.
    CHECK(peak_allocated < 16);

    // The actual recycling claim, and the one worth asserting: free + allocated is every buffer slot the manager has
    // ever created, pooled or live. Without reuse that would be one slot per few records -- tens of thousands -- and
    // the manager would have run out of indices (there are 4095) long before this loop finished.
    //
    // Note this is the right question to ask, where "is anything sitting in the pool" is not. The free list is
    // legitimately empty much of the time, because a reclaimed buffer is handed straight back out to the slot being
    // written next; an empty pool means recycling is keeping up, not that it failed.
    CHECK(mgr->free_buffers() + mgr->allocated_buffers() < 16);
  }

  SECTION("A location from a reclaimed slot stops resolving instead of reading a stranger's bytes") {
    // This is the property generation-tagged ids exist for. Before them a stale id resolved to whichever buffer had
    // since taken over that slot, and the interpreter happily executed whatever was there -- silently, with no
    // failure anywhere to notice.
    tvm::TraceBuffer tb(mgr, 2);

    const auto stale = record_one(tb, 0xBEEF);
    REQUIRE(mgr->find(stale.code.id) != nullptr);

    // Fill the rest of this slot so the ring moves on, then reclaim it.
    for (u16 i = 1; i < PER_SLOT; ++i) record_one(tb, i);
    REQUIRE(tb.cursor().slot == 1);
    tb.acknowledge({1, 0});

    // Keep recording so the reclaimed buffers get handed back out. Either the stale id names a slot that is empty,
    // or one whose index has since been re-issued to somebody else; a bare index could not tell the second case from
    // a live reference, and that is the one that used to read a stranger's bytes.
    for (u16 i = 0; i < 64; ++i) record_one(tb, i);

    CHECK(mgr->find(stale.code.id) == nullptr);

    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    blaster.run(stale);
    CHECK(blaster.stopped());
    CHECK(blaster.csrs().F == 1);
    CHECK(blaster.stop_cause() == tvm::StopCause::InvalidIBuffer);
  }

  SECTION("Records in the live slot still replay after the ring has turned") {
    tvm::TraceBuffer tb(mgr, 2);
    for (std::size_t i = 0; i < std::size_t(PER_SLOT) * 2 + 5; ++i) {
      record_one(tb, u16(i));
      drain_behind(tb);
    }

    // Whatever is still outstanding has to be runnable -- reclaiming everything behind it must not have disturbed
    // the buffers it points at.
    const auto live = record_one(tb, 0x1234);
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    blaster.run(live);
    CHECK(blaster.stopped());
    CHECK(blaster.csrs().F == 0);
    CHECK(blaster.stop_cause() == tvm::StopCause::None);
    // The body set DS from the record, and the driver seeded DP from its location-buffer entry.
    CHECK(blaster.regs().DS == 4);
    CHECK(blaster.regs().DP.hi == live.data.id.value);
    CHECK(blaster.regs().DP.lo == live.data.offset);
  }

  SECTION("Location buffers are released with their slot and re-acquired on demand") {
    tvm::TraceBuffer tb(mgr, 2);
    const auto idle = mgr->allocated_buffers();

    record_one(tb, 1);
    const auto recording = mgr->allocated_buffers();
    CHECK(recording > idle); // a location buffer and a data buffer had to come from somewhere

    for (u16 i = 1; i < PER_SLOT; ++i) record_one(tb, i);
    REQUIRE(tb.cursor().slot == 1);
    tb.acknowledge({1, 0});

    // Reclaiming the slot hands its location buffer back too -- it is a full 64 KiB to hold eight bytes per program,
    // so a slot nobody is using must not keep one.
    CHECK(mgr->free_buffers() > 0);
    CHECK(mgr->allocated_buffers() < recording);
  }
}

TEST_CASE("tvm::TraceBuffer: sustained load through the recorder",
          "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  // The same wrap-and-reclaim cycle, but with records built the way a running system builds them: real writes
  // through a real Target, filed by a Recorder. Exercises the payload path and, at the end, that an undo still
  // works after the ring has turned several times underneath it.
  System::Configuration root_cfg{{.basename = "/", .compatible = System::compatible}};
  Dense::Configuration mem_cfg{Device::Configuration{.basename = "memory", .compatible = Dense::compatible}, 0x00,
                               AddressSpan(0x0000, 0xffff)};
  trace::BufferDevice::Configuration tb_cfg{Device::Configuration{.basename = "trace"}, 2};

  auto sys = std::make_unique<System>(root_cfg);
  auto *mem = sys->make_device<Dense>(mem_cfg);
  auto *tbdev = sys->make_device<trace::BufferDevice>(tb_cfg);
  sys->initialize();

  auto &tb = tbdev->buffer();
  tb.trace(mem->id(), true);
  constexpr Device::ID CPU{1};
  const Operation wrote_op(Operation::Type::Standard, Operation::Kind::data, CPU);
  const Operation app(Operation::Type::Application, Operation::Kind::data);

  auto poke = [&](Address at, u16 v) { ((Target *)mem)->write<u16, bits::host_is_le>(at, v, app); };
  auto peek = [&](Address at) { return ((Target *)mem)->read<u16, bits::host_is_le>(at, app).second; };

  // Two slots' worth plus change, reclaiming behind us the whole way.
  const std::size_t total = std::size_t(PER_SLOT) * 2 + 32;
  for (std::size_t i = 0; i < total; ++i) {
    tb.begin(CPU);
    ((Target *)mem)->write<u16, bits::host_is_le>(0x1000, u16(i), wrote_op);
    tb.commit(CPU);
    drain_behind(tb);
  }
  CHECK(tb.instruction_count() == total);

  // One more instruction, then undo just that one. Everything before it has been reclaimed; this must not care.
  poke(0x2000, 0xAAAA);
  tb.begin(CPU);
  ((Target *)mem)->write<u16, bits::host_is_le>(0x2000, 0x5555, wrote_op);
  const auto last = tb.commit(CPU);
  REQUIRE(peek(0x2000) == 0x5555);

  auto blaster = sys->make_trace_interpreter();
  blaster->run(last);
  CHECK(blaster->csrs().F == 0);
  CHECK(peek(0x2000) == 0xAAAA);
  // And forward again, because the record is its own inverse either way round.
  blaster->run(last);
  CHECK(peek(0x2000) == 0x5555);
}
