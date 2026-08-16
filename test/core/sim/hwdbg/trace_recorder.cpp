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
#include "core/sim/debugger/trace_recorder.hpp"
#include <array>
#include "core/sim/api/trace.hpp"
#include "core/sim/debugger/trace_device.hpp"
#include <catch.hpp>
#include "core/sim/debugger/tvm_interpreter.hpp"
#include "core/sim/debugger/tvm_tracebuffer.hpp"
#include "core/sim/memory/ram/dense.hpp"
#include "core/sim/memory/ram/sparse.hpp"
#include "core/sim/system.hpp"

namespace {

// A system with nothing in it but one RAM. The recorder does not need a CPU -- it only needs a traced Target to aim
// replayed writes at, and an initiator ID to file them under.
auto make_system() {
  System::Configuration root_cfg{{.basename = "/", .compatible = System::compatible}};
  Dense::Configuration mem_cfg{
      Device::Configuration{.basename = "memory", .compatible = Dense::compatible},
      0x00,
      AddressSpan(0x0000, 0xffff),
  };
  auto system = std::make_unique<System>(root_cfg);
  auto *mem = system->make_device<Dense>(mem_cfg);
  system->initialize();
  return std::make_tuple(std::move(system), mem);
}

const Operation app(Operation::Type::Application, Operation::Kind::data);

// Read straight out of the target, bypassing anything that might itself be traced.
u16 peek(Target *mem, Address at) { return mem->read<u16, bits::host_is_le>(at, app).second; }
void poke(Target *mem, Address at, u16 v) { mem->write<u16, bits::host_is_le>(at, v, app); }

} // namespace

TEST_CASE("trace::Recorder: emit_write()", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto [sys, mem] = make_system();
  auto mgr = sys->buffer_manager();
  tvm::TraceBuffer tb(mgr);

  // Stand in for the CPU that caused the writes. Nothing needs it to be a real device -- it is only a filing key.
  constexpr Device::ID CPU{1};
  constexpr Address ADDR = 0x1234;
  constexpr u16 OLD = 0xAAAA, NEW = 0x5678;
  const std::array<u8, 2> old_bytes{0xAA, 0xAA}, new_bytes{0x56, 0x78};

  trace::Recorder rec(&tb, mem->id());
  tb.trace(mem->id());
  const Operation emit_write_op(Operation::Type::Standard, Operation::Kind::data, CPU);

  SECTION("A recorded write replays forward, then undoes itself") {
    // This is the property the whole XOR encoding exists for: one record, replayed twice, lands back where it started.
    poke(mem, ADDR, OLD);

    tb.begin(CPU);
    rec.emit_write(emit_write_op, ADDR, old_bytes, new_bytes);
    auto loc = tb.commit(CPU);

    auto blaster = sys->make_trace_interpreter();

    blaster->run(loc);
    CHECK(blaster->stop_cause() == tvm::StopCause::None);
    CHECK(blaster->csrs().F == 0);
    CHECK(peek(mem, ADDR) == NEW); // forward: old -> new

    blaster->run(loc);
    CHECK(peek(mem, ADDR) == OLD); // and back again, from the same bytes
  }

  SECTION("Only the recorded address moves") {
    poke(mem, ADDR, OLD);
    poke(mem, ADDR + 2, 0x9999);

    tb.begin(CPU);
    rec.emit_write(emit_write_op, ADDR, old_bytes, new_bytes);
    auto loc = tb.commit(CPU);

    sys->make_trace_interpreter()->run(loc);
    CHECK(peek(mem, ADDR) == NEW);
    CHECK(peek(mem, ADDR + 2) == 0x9999);
  }

  SECTION("Several writes in one recording all replay") {
    constexpr Address SECOND = 0x2000;
    poke(mem, ADDR, OLD);
    poke(mem, SECOND, 0x1111);
    const std::array<u8, 2> second_old{0x11, 0x11}, second_new{0x22, 0x22};

    tb.begin(CPU);
    rec.emit_write(emit_write_op, ADDR, old_bytes, new_bytes);
    rec.emit_write(emit_write_op, SECOND, second_old, second_new);
    auto loc = tb.commit(CPU);

    auto blaster = sys->make_trace_interpreter();
    blaster->run(loc);
    CHECK(peek(mem, ADDR) == NEW);
    CHECK(peek(mem, SECOND) == 0x2222);

    // Both undo together -- one instruction's worth of writes is one unit of history.
    blaster->run(loc);
    CHECK(peek(mem, ADDR) == OLD);
    CHECK(peek(mem, SECOND) == 0x1111);
  }

  SECTION("An untraced device records nothing") {
    tb.trace(mem->id(), false);
    CHECK_FALSE(rec.traced());
    poke(mem, ADDR, OLD);

    tb.begin(CPU);
    rec.emit_write(emit_write_op, ADDR, old_bytes, new_bytes);
    auto loc = tb.commit(CPU);

    sys->make_trace_interpreter()->run(loc);
    CHECK(peek(mem, ADDR) == OLD); // the program was just a HALT
  }

  SECTION("A default-constructed recorder is inert") {
    trace::Recorder inert; // never bound to a buffer
    CHECK_FALSE(inert.traced());
    tb.begin(CPU);
    inert.emit_write(emit_write_op, ADDR, old_bytes, new_bytes); // must not dereference a null buffer
    auto loc = tb.commit(CPU);

    poke(mem, ADDR, OLD);
    sys->make_trace_interpreter()->run(loc);
    CHECK(peek(mem, ADDR) == OLD);
  }

  SECTION("The enable bit lives on the buffer, not the recorder") {
    // Two recorders over the same device must agree, and toggling through the buffer must reach both -- that is the
    // point of there being one representation of "off".
    trace::Recorder other(&tb, mem->id());
    CHECK(rec.traced());
    CHECK(other.traced());

    tb.trace(mem->id(), false);
    CHECK_FALSE(rec.traced());
    CHECK_FALSE(other.traced());

    tb.trace(mem->id(), true);
    CHECK(rec.traced());
    CHECK(other.traced());

    // And it is per-device, not global.
    CHECK_FALSE(tb.traced(Device::ID{200}));
  }

  SECTION("BufferInternal accesses are not recorded") {
    // Replay writes come back through the same Target::write. Recording them would append the trace to itself.
    poke(mem, ADDR, OLD);
    const Operation internal(Operation::Type::BufferInternal, Operation::Kind::data, CPU);

    tb.begin(CPU);
    rec.emit_write(internal, ADDR, old_bytes, new_bytes);
    auto loc = tb.commit(CPU);

    sys->make_trace_interpreter()->run(loc);
    CHECK(peek(mem, ADDR) == OLD);
  }

  SECTION("A write with no recording open is dropped, not asserted") {
    // Device init and UI pokes between instructions both land here. There is nowhere to file them.
    CHECK_FALSE(tb.is_recording(CPU));
    rec.emit_write(emit_write_op, ADDR, old_bytes, new_bytes);

    // And the recorder still works afterwards, so the dropped write left no half-built state behind.
    poke(mem, ADDR, OLD);
    tb.begin(CPU);
    rec.emit_write(emit_write_op, ADDR, old_bytes, new_bytes);
    auto loc = tb.commit(CPU);
    sys->make_trace_interpreter()->run(loc);
    CHECK(peek(mem, ADDR) == NEW);
  }

  SECTION("A write whose initiator has no recording open is dropped") {
    // CPU is recording, but this access came from a different initiator.
    constexpr Device::ID OTHER{2};
    poke(mem, ADDR, OLD);
    const Operation from_other(Operation::Type::Standard, Operation::Kind::data, OTHER);

    tb.begin(CPU);
    rec.emit_write(from_other, ADDR, old_bytes, new_bytes);
    auto loc = tb.commit(CPU);

    sys->make_trace_interpreter()->run(loc);
    CHECK(peek(mem, ADDR) == OLD);
  }

  SECTION("Writes are filed under the initiator, not the emitting device") {
    // Two initiators recording at once. Each one's write must end up in its own recording, so replaying only CPU's
    // program moves only CPU's address.
    constexpr Device::ID OTHER{2};
    constexpr Address OTHER_ADDR = 0x3000;
    poke(mem, ADDR, OLD);
    poke(mem, OTHER_ADDR, 0x4444);
    const std::array<u8, 2> other_old{0x44, 0x44}, other_new{0x77, 0x77};

    tb.begin(CPU);
    tb.begin(OTHER);
    rec.emit_write(emit_write_op, ADDR, old_bytes, new_bytes);
    rec.emit_write(Operation(Operation::Type::Standard, Operation::Kind::data, OTHER), OTHER_ADDR, other_old,
                   other_new);
    auto cpu_loc = tb.commit(CPU);
    auto other_loc = tb.commit(OTHER);

    auto blaster = sys->make_trace_interpreter();
    blaster->run(cpu_loc);
    CHECK(peek(mem, ADDR) == NEW);
    CHECK(peek(mem, OTHER_ADDR) == 0x4444); // untouched by CPU's program

    blaster->run(other_loc);
    CHECK(peek(mem, OTHER_ADDR) == 0x7777);
  }

  SECTION("Mismatched span lengths use the shorter one") {
    poke(mem, ADDR, OLD);
    const std::array<u8, 1> just_one{0x56};

    tb.begin(CPU);
    rec.emit_write(emit_write_op, ADDR, old_bytes, just_one);
    auto loc = tb.commit(CPU);

    sys->make_trace_interpreter()->run(loc);
    // Only the first byte was folded; the second keeps its old value. Memory is big-endian here, so the high byte is
    // the one that moved.
    CHECK(peek(mem, ADDR) == 0x56AA);
  }

  SECTION("An empty write is a no-op") {
    poke(mem, ADDR, OLD);
    tb.begin(CPU);
    rec.emit_write(emit_write_op, ADDR, {}, {});
    auto loc = tb.commit(CPU);

    sys->make_trace_interpreter()->run(loc);
    CHECK(peek(mem, ADDR) == OLD);
  }
}

TEST_CASE("trace::BufferDevice: discovery and binding", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  // Build a system that contains a trace buffer device, so binding happens through System::initialize() rather than
  // by handing a buffer to bind_recorders() by hand.
  System::Configuration root_cfg{{.basename = "/", .compatible = System::compatible}};
  Dense::Configuration mem_cfg{
      Device::Configuration{.basename = "memory", .compatible = Dense::compatible},
      0x00,
      AddressSpan(0x0000, 0xffff),
  };
  trace::BufferDevice::Configuration tb_cfg{Device::Configuration{.basename = "trace"}, 4};

  auto sys = std::make_unique<System>(root_cfg);
  auto *mem = sys->make_device<Dense>(mem_cfg);
  auto *tbdev = sys->make_device<trace::BufferDevice>(tb_cfg);
  sys->initialize();

  constexpr Device::ID CPU{1};
  constexpr Address ADDR = 0x1234;
  constexpr u16 OLD = 0xAAAA, NEW = 0x5678;

  SECTION("The buffer is reachable as a capability, not just via a System method") {
    auto *found = mem->capability<trace::BufferDevice>();
    CHECK(found == nullptr); // a RAM is not a trace buffer

    found = tbdev->capability<trace::BufferDevice>();
    REQUIRE(found != nullptr);
    CHECK(found->instruction_count() == 0);
  }

  SECTION("The buffer device is not itself Traceable") {
    // It holds no simulated state, and a buffer recording its own writes would feed itself.
    CHECK(tbdev->capability<Traceable>() == nullptr);
    CHECK(mem->capability<Traceable>() != nullptr);
  }

  SECTION("initialize() bound a Recorder to every Traceable") {
    // Nothing called bind_recorders here. If binding did not happen, the write below is silently dropped.
    auto *found = tbdev->capability<trace::BufferDevice>();
    REQUIRE(found != nullptr);
    CHECK_FALSE(found->traced(mem->id())); // devices start switched off
    found->trace(mem->id(), true);
    CHECK(found->traced(mem->id()));

    poke(mem, ADDR, OLD);

    auto &tb = tbdev->buffer();
    tb.begin(CPU);
    // Go through the device's own write path, so this exercises the Recorder that initialize() bound.
    ((Target *)mem)->write<u16, bits::host_is_le>(ADDR, NEW, Operation(Operation::Type::Standard,
                                                                       Operation::Kind::data, CPU));
    auto loc = tb.commit(CPU);
    CHECK(peek(mem, ADDR) == NEW);

    // Replaying the recorded program undoes the write, which is only possible if the device really recorded it.
    sys->make_trace_interpreter()->run(loc);
    CHECK(peek(mem, ADDR) == OLD);
  }

  SECTION("An untraced device's writes are not recorded") {
    auto &tb = tbdev->buffer();
    poke(mem, ADDR, OLD);
    tb.begin(CPU);
    ((Target *)mem)->write<u16, bits::host_is_le>(ADDR, NEW, Operation(Operation::Type::Standard,
                                                                       Operation::Kind::data, CPU));
    auto loc = tb.commit(CPU);
    CHECK(peek(mem, ADDR) == NEW);

    sys->make_trace_interpreter()->run(loc);
    CHECK(peek(mem, ADDR) == NEW); // nothing to undo
  }
}

TEST_CASE("trace::Recorder: devices whose prior bytes must be fetched",
          "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  // Sparse is paged, so its previous contents are not sitting in a flat array the way Dense's are. It exercises the
  // emit_write() overload that reads the prior bytes straight into the trace record.
  System::Configuration root_cfg{{.basename = "/", .compatible = System::compatible}};
  Sparse::Configuration mem_cfg{
      Device::Configuration{.basename = "memory", .compatible = Sparse::compatible},
      0xCC, // distinctive fill, so an unwritten page is recognisable
      AddressSpan(0x0000, 0xffff),
  };
  trace::BufferDevice::Configuration tb_cfg{Device::Configuration{.basename = "trace"}, 4};

  auto sys = std::make_unique<System>(root_cfg);
  auto *mem = sys->make_device<Sparse>(mem_cfg);
  auto *tbdev = sys->make_device<trace::BufferDevice>(tb_cfg);
  sys->initialize();

  auto &tb = tbdev->buffer();
  tb.trace(mem->id(), true);

  constexpr Device::ID CPU{1};
  constexpr Address ADDR = 0x4321;
  constexpr u16 OLD = 0x1234, NEW = 0x5678;
  const Operation emit_write_op(Operation::Type::Standard, Operation::Kind::data, CPU);

  SECTION("A paged write replays forward and undoes itself") {
    poke((Target *)mem, ADDR, OLD);

    tb.begin(CPU);
    ((Target *)mem)->write<u16, bits::host_is_le>(ADDR, NEW, emit_write_op);
    auto loc = tb.commit(CPU);
    CHECK(peek((Target *)mem, ADDR) == NEW);

    auto blaster = sys->make_trace_interpreter();
    blaster->run(loc);
    CHECK(peek((Target *)mem, ADDR) == OLD);
    blaster->run(loc);
    CHECK(peek((Target *)mem, ADDR) == NEW);
  }

  SECTION("Undo of a write to a never-touched page restores the fill value") {
    // The prior bytes come from a page that was never allocated, so the pool reads back the configured fill. That is
    // the correct thing to restore -- anything else would invent state the machine never had.
    constexpr Address VIRGIN = 0x8000;
    CHECK(peek((Target *)mem, VIRGIN) == 0xCCCC);

    tb.begin(CPU);
    ((Target *)mem)->write<u16, bits::host_is_le>(VIRGIN, NEW, emit_write_op);
    auto loc = tb.commit(CPU);
    CHECK(peek((Target *)mem, VIRGIN) == NEW);

    sys->make_trace_interpreter()->run(loc);
    CHECK(peek((Target *)mem, VIRGIN) == 0xCCCC);
  }

  SECTION("An untraced Sparse never pays for the prior-byte read") {
    // The callback is only invoked once every filter has passed, so switching the device off skips the pool read
    // entirely rather than reading and discarding.
    tb.trace(mem->id(), false);
    poke((Target *)mem, ADDR, OLD);

    tb.begin(CPU);
    ((Target *)mem)->write<u16, bits::host_is_le>(ADDR, NEW, emit_write_op);
    auto loc = tb.commit(CPU);

    sys->make_trace_interpreter()->run(loc);
    CHECK(peek((Target *)mem, ADDR) == NEW); // nothing was recorded, so nothing to undo
  }
}

TEST_CASE("trace::Recorder: repeated writes de-duplicate", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  // The point of keeping absolute DP setup in the prefix: bodies carry no per-instance addresses, so an instruction
  // that writes the same places the same way produces a byte-identical body and collapses into a CALL.
  System::Configuration root_cfg{{.basename = "/", .compatible = System::compatible}};
  Dense::Configuration mem_cfg{
      Device::Configuration{.basename = "memory", .compatible = Dense::compatible},
      0x00,
      AddressSpan(0x0000, 0xffff),
  };
  trace::BufferDevice::Configuration tb_cfg{Device::Configuration{.basename = "trace"}, 4};

  auto sys = std::make_unique<System>(root_cfg);
  auto *mem = sys->make_device<Dense>(mem_cfg);
  auto *tbdev = sys->make_device<trace::BufferDevice>(tb_cfg);
  sys->initialize();

  auto &tb = tbdev->buffer();
  tb.trace(mem->id(), true);
  constexpr Device::ID CPU{1};
  const Operation emit_write_op(Operation::Type::Standard, Operation::Kind::data, CPU);

  // Stand in for an instruction: two writes to fixed addresses, exactly what a register-bank update looks like.
  auto one_instruction = [&](u16 a, u16 b) {
    tb.begin(CPU);
    ((Target *)mem)->write<u16, bits::host_is_le>(0x10, a, emit_write_op);
    ((Target *)mem)->write<u16, bits::host_is_le>(0x20, b, emit_write_op);
    return tb.commit(CPU);
  };

  SECTION("The same shape of instruction promotes to a stencil") {
    CHECK(tb.stencil_count() == 0);

    one_instruction(0x1111, 0x2222);
    // First sighting: nothing to match against yet.
    CHECK(tb.stencil_count() == 0);
    CHECK(tb.pending_count() == 1);

    one_instruction(0x3333, 0x4444);
    // Second sighting of an identical body. Under the old always-absolute-LDP encoding this could never happen: the
    // body embedded a data-buffer id that moved every time.
    CHECK(tb.stencil_count() == 1);

    one_instruction(0x5555, 0x6666);
    CHECK(tb.stencil_count() == 1); // reused, not re-promoted
  }

  SECTION("Stenciled programs still replay correctly") {
    // The body became a CALL, so this also checks the prefix anchors DP correctly for a body it no longer contains.
    poke((Target *)mem, 0x10, 0xAAAA);
    poke((Target *)mem, 0x20, 0xBBBB);
    one_instruction(0x1111, 0x2222);

    poke((Target *)mem, 0x10, 0xCCCC);
    poke((Target *)mem, 0x20, 0xDDDD);
    auto loc = one_instruction(0x1111, 0x2222);
    REQUIRE(tb.stencil_count() == 1); // this one really did go through the stencil path

    CHECK(peek((Target *)mem, 0x10) == 0x1111);
    CHECK(peek((Target *)mem, 0x20) == 0x2222);

    auto blaster = sys->make_trace_interpreter();
    blaster->run(loc);
    CHECK(peek((Target *)mem, 0x10) == 0xCCCC); // undone to the values from just before this instruction
    CHECK(peek((Target *)mem, 0x20) == 0xDDDD);
  }

  SECTION("Writes to different addresses do not collide") {
    one_instruction(0x1111, 0x2222);
    tb.begin(CPU);
    ((Target *)mem)->write<u16, bits::host_is_le>(0x30, 0x9999, emit_write_op); // different address -> different body
    tb.commit(CPU);
    CHECK(tb.stencil_count() == 0);
    CHECK(tb.pending_count() == 2);
  }

  SECTION("Pending hashes stay bounded") {
    // Every body here is unique, which without a cap would add one entry per program forever.
    for (u16 i = 0; i < 64; ++i) {
      tb.begin(CPU);
      ((Target *)mem)->write<u16, bits::host_is_le>(0x100 + i * 2, i, emit_write_op);
      tb.commit(CPU);
    }
    CHECK(tb.pending_count() <= tvm::TraceBuffer::MAX_PENDING_HASHES);
  }

  SECTION("Footprint reports reports promotion savings") {
    const auto empty = tb.footprint();
    CHECK(empty.programs == 0);
    CHECK(empty.total() == 0);
    // Defined as 0 rather than dividing by zero when nothing has been recorded.
    CHECK(empty.compression_ratio() == 0.0);

    // Twenty instructions of identical shape. The first should be inlined, the remaining are calls.
    for (u16 i = 0; i < 20; ++i) one_instruction(i, (u16)(i + 1));

    const auto f = tb.footprint();
    CHECK(f.programs == 20);
    CHECK(f.programs == tb.instruction_count());
    REQUIRE(tb.stencil_count() == 1);

    // Always inlining should have a cost.
    CHECK(f.code < f.code_if_inlined);
    CHECK(f.stencils > 0);
    // Check that the cost of stencils + code is less than the always-inlined case
    CHECK(f.total() < f.total_if_inlined());
    CHECK(f.compression_ratio() > 1.0);

    // Payloads and location entries are untouched by promotion, so they sit on both sides unchanged.
    CHECK(f.data > 0);
    CHECK(f.total() - f.data - f.locations() == f.code + f.stencils);

    // Reserved is not the same question as written: buffer_footprint counts whole buffers, so it is a multiple of the
    // buffer size and never smaller than the bytes actually put in them.
    CHECK(tb.buffer_footprint() % pepp::bts::Buffer::SIZE == 0);
    CHECK(tb.buffer_footprint() >= f.total());

    // Reset puts the counters back to the state the top of this section asserted, so a later measurement covers only
    // what follows it.
    tb.reset_footprint();
    const auto cleared = tb.footprint();
    CHECK(cleared.programs == 0);
    CHECK(cleared.total() == 0);
    CHECK(cleared.total_if_inlined() == 0);
    CHECK(cleared.compression_ratio() == 0.0);
    CHECK(tb.instruction_count() == 0);

    // Statistics only. The buffer still holds everything it did a moment ago -- the stencil survived, so the next
    // instruction of this shape is still a CALL -- and counting resumes from zero rather than from nothing.
    CHECK(tb.stencil_count() == 1);
    CHECK(tb.buffer_footprint() >= f.total());
    one_instruction(0xAB, 0xCD);
    const auto after = tb.footprint();
    CHECK(after.programs == 1);
    CHECK(after.data > 0);
    // Promoted on the first sighting after the reset, which it could only do by reusing the surviving stencil.
    CHECK(after.code < after.code_if_inlined);
  }

  SECTION("No compression for unique bodies") {
    // These bodies differ in shape because each record only differs by an address, which will be "solved" via SETMEMDX.
    // Force each instruction to have a different number of writes to bypass that deduplication mechanism.
    for (u16 i = 1; i <= 20; ++i) {
      tb.begin(CPU);
      for (u16 w = 0; w < i; ++w) ((Target *)mem)->write<u16, bits::host_is_le>(0x100 + w * 2, w, emit_write_op);
      tb.commit(CPU);
    }

    const auto f = tb.footprint();
    CHECK(tb.stencil_count() == 0);
    CHECK(f.stencils == 0);
    // Nothing was replaced, so both sides agree exactly and the ratio is exactly 1.
    CHECK(f.code == f.code_if_inlined);
    CHECK(f.compression_ratio() == 1.0);
    CHECK(f.bytes_per_program() == f.bytes_per_program_if_inlined());
  }
}

TEST_CASE("System::initialize: at most one trace buffer, wherever it sits",
          "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  // The device that finds the buffer is not necessarily the last one visited. This used to throw for any device
  // initialized after the buffer, because the duplicate check tested the wrong pointer -- and it went unnoticed
  // because every existing test happens to create its buffer last.
  System::Configuration root_cfg{{.basename = "/", .compatible = System::compatible}};
  Dense::Configuration mem_cfg{
      Device::Configuration{.basename = "memory", .compatible = Dense::compatible},
      0x00,
      AddressSpan(0x0000, 0xffff),
  };

  SECTION("A buffer followed by other devices initializes") {
    auto sys = std::make_unique<System>(root_cfg);
    // Buffer first, then a device after it. This is the ordering that used to throw.
    auto *tbdev = sys->make_device<trace::BufferDevice>(
        trace::BufferDevice::Configuration{Device::Configuration{.basename = "trace"}, 4});
    auto *mem = sys->make_device<Dense>(mem_cfg);
    REQUIRE_NOTHROW(sys->initialize());

    // The buffer was adopted, not merely tolerated. Enabling tracing for the RAM only reaches it if initialize()
    // bound a recorder aimed at this buffer, so this distinguishes "did not throw" from "actually finished".
    tbdev->trace(mem->id(), true);
    auto *traceable = mem->capability<Traceable>();
    REQUIRE(traceable != nullptr);
    CHECK(traceable->traced());
  }

  SECTION("Two buffers separated by another device are still refused") {
    auto sys = std::make_unique<System>(root_cfg);
    // Non-adjacent on purpose: the old check reset its record of "seen one" on every non-buffer device, so a second
    // buffer with anything in between slipped through.
    sys->make_device<trace::BufferDevice>(
        trace::BufferDevice::Configuration{Device::Configuration{.basename = "trace0"}, 4});
    sys->make_device<Dense>(mem_cfg);
    sys->make_device<trace::BufferDevice>(
        trace::BufferDevice::Configuration{Device::Configuration{.basename = "trace1"}, 4});
    CHECK_THROWS_AS(sys->initialize(), std::logic_error);
  }
}
