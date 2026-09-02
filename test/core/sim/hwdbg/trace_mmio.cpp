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
#include <memory>
#include <vector>

#include <catch.hpp>
#include "core/math/bitmanip/enums.hpp"
#include "core/sim/api/memory.hpp"
#include "core/sim/debugger/trace_device.hpp"
#include "core/sim/debugger/tvm_apply_backend.hpp"
#include "core/sim/debugger/tvm_interpreter.hpp"
#include "core/sim/debugger/tvm_tracebuffer.hpp"
#include "core/sim/memory/errors.hpp"
#include "core/sim/memory/io/fifo.hpp"
#include "core/sim/memory/ram/dense.hpp"
#include "core/sim/system.hpp"

namespace {
using namespace bits;

constexpr auto BIDI = FIFORegister::Direction::Input | FIFORegister::Direction::Output;

// Stands in for the CPU that caused the accesses.
constexpr Device::ID CPU{1};
// Deliberately wider than 16 bits. An MMIO record carries its address as four payload bytes ahead of the data, so a
// port whose high half is non-zero is the only way to notice if that half is dropped on the way through.
constexpr Address PORT = 0x1FC15;
// Somewhere in the RAM that shares the system with the port, for the case where one instruction touches both.
constexpr Address RAM = 0x1234;
// What one MMIO record costs in the data chain: the four address bytes plus its one-byte payload.
constexpr std::size_t RECORD_BYTES = tvm::MMIO_PROLOGUE_BYTES + 1;

const Operation cpu_op(Operation::Type::Standard, Operation::Kind::data, CPU);
const Operation app_op(Operation::Type::Application, Operation::Kind::data, CPU);
const Operation internal_op(Operation::Type::BufferInternal, Operation::Kind::data, CPU);

// A system holding one FIFO register and a trace buffer.
struct Harness {
  std::unique_ptr<System> sys;
  FIFORegister *fifo = nullptr;
  Dense *mem = nullptr;
  trace::BufferDevice *tbdev = nullptr;

  tvm::TraceBuffer &tb() { return tbdev->buffer(); }
  Target *port() { return static_cast<Target *>(fifo); }

  u8 read(Operation op = cpu_op) { return port()->read<u8>(PORT, op).second; }
  void write(u8 value, Operation op = cpu_op) { port()->write<u8>(PORT, value, op); }
  // The byte the next read would consume. An Application access does not advance the queue and is never recorded, so
  // this observes the read position without being the thing under test.
  u8 peek() { return read(app_op); }

  // Straight at the RAM, bypassing anything that might itself be traced.
  u16 peek_ram() { return ((Target *)mem)->read<u16, bits::host_is_le>(RAM, app_op).second; }
  void poke_ram(u16 v) { ((Target *)mem)->write<u16, bits::host_is_le>(RAM, v, app_op); }
  void write_ram(u16 v) { ((Target *)mem)->write<u16, bits::host_is_le>(RAM, v, cpu_op); }

  // One instruction's worth of accesses: open a recording, run `body`, close it.
  template <typename F> tvm::ProgramLocation instruction(F &&body) {
    tb().begin(CPU);
    body();
    return tb().commit(CPU);
  }

  // Replay one recorded program through the real apply backend. Backward is the undo direction. A stop means the
  // program never reached its MMIO op, which would make every assertion after it vacuous.
  void replay(tvm::ProgramLocation loc, tvm::Direction dir) {
    auto blaster = sys->make_trace_interpreter();
    blaster->backend().set_direction(dir);
    blaster->run(loc);
    REQUIRE(blaster->stop_cause() == tvm::StopCause::None);
  }

  // Bytes committed to data chains so far. Records carry their payload here and nowhere else, so a delta across one
  // instruction counts the records it produced without depending on what replaying them does.
  std::size_t recorded_bytes() { return tb().footprint().data; }
};

Harness make_harness(FIFORegister::Direction direction, FailPolicy policy = FailPolicy::RaiseError, u8 fill = 0x00) {
  System::Configuration root_cfg{{.basename = "/", .compatible = System::compatible}};
  FIFORegister::Configuration fifo_cfg{
      Device::Configuration{.basename = "port", .compatible = FIFORegister::compatible}};
  fifo_cfg.span = AddressSpan(PORT, PORT);
  fifo_cfg.direction = direction;
  fifo_cfg.fail_policy = policy;
  fifo_cfg.fill = fill;
  // A RAM alongside the port, so a recording can hold an ordinary write and an MMIO record at once. Its span is wide
  // enough that bind_recorders() picks the address-in-payload encoding for it, which is what main memory really gets.
  Dense::Configuration mem_cfg{
      Device::Configuration{.basename = "memory", .compatible = Dense::compatible},
      0x00,
      AddressSpan(0x0000, 0xffff),
  };
  trace::BufferDevice::Configuration tb_cfg{Device::Configuration{.basename = "trace"}, 4};

  Harness h;
  h.sys = std::make_unique<System>(root_cfg);
  h.fifo = h.sys->make_device<FIFORegister>(fifo_cfg);
  h.mem = h.sys->make_device<Dense>(mem_cfg);
  h.tbdev = h.sys->make_device<trace::BufferDevice>(tb_cfg);
  h.sys->initialize();
  h.tbdev->trace(h.fifo->id(), true);
  h.tbdev->trace(h.mem->id(), true);
  return h;
}

} // namespace

TEST_CASE("trace::Recorder: MMIO on a bidirectional FIFO", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto h = make_harness(BIDI);

  SECTION("A recorded write replays forward, then undoes itself") {
    // Moves the output queue to where it belongs and back.
    auto loc = h.instruction([&] { h.write(0x5A); });
    REQUIRE(h.fifo->output().size() == 1);
    CHECK(h.fifo->output().latest_or(0) == 0x5A);

    h.replay(loc, tvm::Direction::Backward);
    CHECK(h.fifo->output().empty());

    h.replay(loc, tvm::Direction::Forward);
    REQUIRE(h.fifo->output().size() == 1);
    CHECK(h.fifo->output().latest_or(0) == 0x5A);
  }

  SECTION("Undoing a read steps the read position back over the byte") {
    // Undo runs against the device that did the read, so the byte is still queued -- only the position moved, and
    // only the position has to come back. Leaving the byte in place is what lets the machine consume it again when
    // it re-executes the instruction.
    h.fifo->input().push(0xAB);
    h.fifo->input().push(0xCD);
    auto loc = h.instruction([&] { CHECK(h.read() == 0xAB); });
    REQUIRE(h.peek() == 0xCD); // consumed 0xAB and moved on

    h.replay(loc, tvm::Direction::Backward);
    CHECK(h.peek() == 0xAB);            // back where the instruction started
    CHECK(h.fifo->input().size() == 2); // and nothing was taken out of the queue
  }

  SECTION("Replaying a read forward queues the byte it recorded") {
    // The other direction runs against a device that does not have the byte yet, which is what lets register
    // programming pre-queue input straight out of a trace: the record carries the consumed byte by value, so
    // replaying it forward puts that byte back where the machine can read it.
    h.fifo->input().push(0xAB);
    auto loc = h.instruction([&] { CHECK(h.read() == 0xAB); });

    h.fifo->clear(0); // as if the trace were being replayed onto a fresh machine
    REQUIRE(h.fifo->input().empty());

    h.replay(loc, tvm::Direction::Forward);
    REQUIRE(h.fifo->input().size() == 1);
    CHECK(h.fifo->input().at(0) == 0xAB);
    // Forward replay reproduces the execution rather than staging it, so the byte ends up queued *and* consumed.
    // 0x00 is the configured fill, which is what an exhausted queue yields.
    CHECK(h.peek() == 0x00);
  }

  SECTION("Stepping back over a read and forward again leaves the queue as it was") {
    // The two directions touch different state -- undo moves the read position and leaves the byte, redo appends a
    // byte and leaves the position -- so this is where they have to agree. The byte never left the queue, and the
    // machine ends up back where it started rather than looking at a second copy of input it already consumed.
    h.fifo->input().push(0xAB);
    h.fifo->input().push(0xCD);
    auto loc = h.instruction([&] { CHECK(h.read() == 0xAB); });
    REQUIRE(h.fifo->input().size() == 2);
    REQUIRE(h.peek() == 0xCD);

    h.replay(loc, tvm::Direction::Backward);
    h.replay(loc, tvm::Direction::Forward);

    CHECK(h.fifo->input().size() == 2);
    CHECK(h.peek() == 0xCD);
  }

  SECTION("Both directions of one instruction are recorded") {
    h.fifo->input().push(0xAB);
    const auto before = h.recorded_bytes();

    auto loc = h.instruction([&] {
      CHECK(h.read() == 0xAB);
      h.write(0x5A);
    });
    CHECK(h.recorded_bytes() - before == 2 * RECORD_BYTES);

    h.replay(loc, tvm::Direction::Backward);
    CHECK(h.fifo->output().empty());
  }

  SECTION("Several writes in one recording undo together") {
    auto loc = h.instruction([&] {
      h.write(0x11);
      h.write(0x22);
    });
    REQUIRE(h.fifo->output().size() == 2);

    h.replay(loc, tvm::Direction::Backward);
    CHECK(h.fifo->output().empty());

    // And redo restores them in the order they were written, not reversed.
    h.replay(loc, tvm::Direction::Forward);
    REQUIRE(h.fifo->output().size() == 2);
    CHECK(h.fifo->output().at(0) == 0x11);
    CHECK(h.fifo->output().at(1) == 0x22);
  }

  SECTION("Identical accesses de-duplicate into one stencil") {
    // The point of putting the address in the payload: the body carries neither the port nor the byte, so two writes
    // to the same port produce byte-identical bodies and the second collapses into a CALL.
    CHECK(h.tb().stencil_count() == 0);

    h.instruction([&] { h.write(0x11); });
    CHECK(h.tb().stencil_count() == 0); // first sighting: nothing to match against yet

    auto second = h.instruction([&] { h.write(0x22); });
    CHECK(h.tb().stencil_count() == 1);
    REQUIRE(h.fifo->output().size() == 2);

    // The CALL still reaches this instruction's own payload rather than the one the template was built from.
    h.replay(second, tvm::Direction::Backward);
    REQUIRE(h.fifo->output().size() == 1);
    CHECK(h.fifo->output().latest_or(0) == 0x11);
  }

  SECTION("An MMIO record shares a data chain with an ordinary write") {
    // Real instructions can touch both DRAM and a FIFO register. Like SETMEMDX, the data size will not equal the
    // stride.
    h.fifo->input().push(0xAB);
    h.poke_ram(0x1111);

    auto loc = h.instruction([&] {
      CHECK(h.read() == 0xAB);
      h.write_ram(0x2222);
      h.write(0x5A);
    });
    REQUIRE(h.peek_ram() == 0x2222);
    REQUIRE(h.fifo->output().size() == 1);

    h.replay(loc, tvm::Direction::Backward);
    CHECK(h.peek_ram() == 0x1111); // the ordinary write found its payload
    CHECK(h.fifo->output().empty());
    CHECK(h.peek() == 0xAB); // and the read position came back with it
  }

  SECTION("A run of instructions unwinds newest-first") {
    // Undone
    // The linear-walk assumption in practice. Undone in reverse order every record finds the byte it wrote sitting on
    // top of the queue, which is the whole reason undo can be a bare pop.
    std::vector<tvm::ProgramLocation> locs;
    for (u8 i = 0; i < 3; ++i) locs.push_back(h.instruction([&, i] { h.write(0x10 + i); }));
    REQUIRE(h.fifo->output().size() == 3);

    for (auto it = locs.rbegin(); it != locs.rend(); ++it) h.replay(*it, tvm::Direction::Backward);
    CHECK(h.fifo->output().empty());
  }

  SECTION("Applying a record without a system stops rather than throws") {
    // A backend with no System cannot resolve the target ID. The refusal has to arrive through the ISA's own failure
    // channel, because run_each sits above this and an exception would unwind past it.
    auto loc = h.instruction([&] { h.write(0x5A); });

    auto mgr = h.sys->buffer_manager();
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    blaster.run(loc);
    CHECK(blaster.stop_cause() == tvm::StopCause::MissingSystem);
    CHECK(h.fifo->output().size() == 1); // and it stopped before touching anything
  }
}

TEST_CASE("trace::Recorder: MMIO on an input-only FIFO", "[scope:core][scope:core.dbg][kind:unit][arch:pep10][!throws]") {
  SECTION("A recorded read undoes itself") {
    auto h = make_harness(FIFORegister::Direction::Input);
    h.fifo->input().push(0xAB);
    h.fifo->input().push(0xCD);
    const auto before = h.recorded_bytes();

    auto loc = h.instruction([&] { CHECK(h.read() == 0xAB); });
    CHECK(h.recorded_bytes() - before == RECORD_BYTES);
    REQUIRE(h.peek() == 0xCD);

    h.replay(loc, tvm::Direction::Backward);
    CHECK(h.peek() == 0xAB);
    CHECK(h.fifo->input().size() == 2);
  }

  SECTION("Several reads in one recording undo together") {
    // One instruction's worth of accesses is one unit of history, so the position walks back over all three in a
    // single replay -- an off-by-one anywhere in the run leaves it pointing at the wrong byte.
    auto h = make_harness(FIFORegister::Direction::Input);
    for (u8 i = 0; i < 4; ++i) h.fifo->input().push(0x10 + i);

    auto loc = h.instruction([&] {
      CHECK(h.read() == 0x10);
      CHECK(h.read() == 0x11);
      CHECK(h.read() == 0x12);
    });
    REQUIRE(h.peek() == 0x13);

    h.replay(loc, tvm::Direction::Backward);
    CHECK(h.peek() == 0x10);
    CHECK(h.fifo->input().size() == 4);
  }

  SECTION("Several reads replay forward in the order they were consumed") {
    // Rebuilding a queue from a trace has to preserve order: the records run serially in program order whichever way
    // the walk is going, so pushing them puts the bytes back the way the machine saw them.
    auto h = make_harness(FIFORegister::Direction::Input);
    for (u8 i = 0; i < 3; ++i) h.fifo->input().push(0x10 + i);

    auto loc = h.instruction([&] {
      CHECK(h.read() == 0x10);
      CHECK(h.read() == 0x11);
      CHECK(h.read() == 0x12);
    });

    h.fifo->clear(0);
    h.replay(loc, tvm::Direction::Forward);

    REQUIRE(h.fifo->input().size() == 3);
    CHECK(h.fifo->input().at(0) == 0x10);
    CHECK(h.fifo->input().at(1) == 0x11);
    CHECK(h.fifo->input().at(2) == 0x12);
    CHECK(h.peek() == 0x00); // the whole run is queued, and the read position is past all of it
  }

  SECTION("A write is ignored, and records nothing") {
    auto h = make_harness(FIFORegister::Direction::Input);
    const auto before = h.recorded_bytes();

    auto loc = h.instruction([&] { h.write(0x5A); });

    CHECK(h.recorded_bytes() == before);
    CHECK(h.fifo->output().empty());
    h.replay(loc, tvm::Direction::Forward);
    CHECK(h.fifo->output().empty()); // nothing to replay, so nothing appears
  }

  SECTION("A read that runs out of input records nothing") {
    // Under RaiseError the read throws before consuming anything, so there is nothing to undo. This also checks the
    // throw left no half-built record behind: commit() has to produce a runnable program either way.
    auto h = make_harness(FIFORegister::Direction::Input, FailPolicy::RaiseError);
    const auto before = h.recorded_bytes();

    h.tb().begin(CPU);
    CHECK_THROWS_AS(h.read(), Error);
    auto loc = h.tb().commit(CPU);

    CHECK(h.recorded_bytes() == before);
    h.replay(loc, tvm::Direction::Backward); // and the empty program still runs to its HALT
  }

  SECTION("An exhausted read under yield_default still records") {
    auto h = make_harness(FIFORegister::Direction::Input, FailPolicy::YieldDefaultValue, 0xFE);
    const auto before = h.recorded_bytes();

    h.instruction([&] { CHECK(h.read() == 0xFE); });

    CHECK(h.recorded_bytes() - before == RECORD_BYTES);
  }
}

TEST_CASE("trace::Recorder: MMIO on an output-only FIFO", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto h = make_harness(FIFORegister::Direction::Output);

  SECTION("A recorded write replays forward, then undoes itself") {
    auto loc = h.instruction([&] { h.write(0x5A); });
    REQUIRE(h.fifo->output().size() == 1);

    h.replay(loc, tvm::Direction::Backward);
    CHECK(h.fifo->output().empty());

    h.replay(loc, tvm::Direction::Forward);
    REQUIRE(h.fifo->output().size() == 1);
    CHECK(h.fifo->output().latest_or(0) == 0x5A);
  }

  SECTION("A read records nothing") {
    // Reading an output register hands back the most recent value written. Nothing is consumed, so there is nothing
    // to put back, and a record here would replay as a second push.
    h.instruction([&] { h.write(0x5A); });
    const auto before = h.recorded_bytes();

    auto loc = h.instruction([&] { CHECK(h.read() == 0x5A); });

    CHECK(h.recorded_bytes() == before);
    h.replay(loc, tvm::Direction::Backward);
    CHECK(h.fifo->output().size() == 1); // the write's own record is in a different program
  }
}

TEST_CASE("trace::Recorder: MMIO records are filtered like every other trace",
          "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto h = make_harness(BIDI);

  SECTION("An untraced device records nothing") {
    h.tbdev->trace(h.fifo->id(), false);
    CHECK_FALSE(h.fifo->traced());
    h.fifo->input().push(0xAB);

    auto loc = h.instruction([&] {
      CHECK(h.read() == 0xAB);
      h.write(0x5A);
    });

    h.replay(loc, tvm::Direction::Backward);
    CHECK(h.fifo->output().size() == 1); // the write happened, but nothing recorded it, so nothing undid it
  }

  SECTION("Application accesses neither consume nor record") {
    h.fifo->input().push(0xAB);
    const auto before = h.recorded_bytes();

    h.instruction([&] {
      CHECK(h.read(app_op) == 0xAB);
      CHECK(h.read(app_op) == 0xAB); // still the same byte
      h.write(0x5A, app_op);
    });

    CHECK(h.recorded_bytes() == before);
    CHECK(h.fifo->output().empty());
  }

  SECTION("BufferInternal accesses are not recorded") {
    h.fifo->input().push(0xAB);
    const auto before = h.recorded_bytes();

    h.instruction([&] {
      CHECK(h.read(internal_op) == 0xAB);
      h.write(0x5A, internal_op);
    });

    CHECK(h.recorded_bytes() == before);
  }

  SECTION("An access with no recording open is dropped, not asserted") {
    // Device init and UI pokes between instructions both land here.
    CHECK_FALSE(h.tb().is_recording(CPU));
    h.write(0x11);
    CHECK(h.recorded_bytes() == 0);

    // And the recorder still works afterwards, so the dropped access left no half-built state behind.
    auto loc = h.instruction([&] { h.write(0x22); });
    REQUIRE(h.fifo->output().size() == 2);

    h.replay(loc, tvm::Direction::Backward);
    REQUIRE(h.fifo->output().size() == 1);
    CHECK(h.fifo->output().latest_or(0) == 0x11); // only the recorded write came back out
  }

  SECTION("An access whose initiator has no recording open is dropped") {
    constexpr Device::ID OTHER{2};
    const Operation from_other(Operation::Type::Standard, Operation::Kind::data, OTHER);
    const auto before = h.recorded_bytes();

    auto loc = h.instruction([&] { h.write(0x5A, from_other); });

    CHECK(h.recorded_bytes() == before);
    h.replay(loc, tvm::Direction::Backward);
    CHECK(h.fifo->output().size() == 1);
  }

  SECTION("Records are filed under the initiator, not the emitting device") {
    // Two initiators recording at once, both writing the one port. Each write belongs to its own program: if they had
    // both landed in CPU's, the first undo below would empty the queue instead of taking one byte off it.
    constexpr Device::ID OTHER{2};
    const Operation from_other(Operation::Type::Standard, Operation::Kind::data, OTHER);

    h.tb().begin(CPU);
    h.tb().begin(OTHER);
    h.write(0x11);
    h.write(0x22, from_other);
    auto cpu_loc = h.tb().commit(CPU);
    auto other_loc = h.tb().commit(OTHER);
    REQUIRE(h.fifo->output().size() == 2);

    h.replay(other_loc, tvm::Direction::Backward);
    CHECK(h.fifo->output().size() == 1);

    h.replay(cpu_loc, tvm::Direction::Backward);
    CHECK(h.fifo->output().empty());
  }
}
