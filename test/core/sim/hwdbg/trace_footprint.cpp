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
#include <array>
#include <catch.hpp>
#include <memory>
#include <spdlog/spdlog.h>

#include "core/arch/pep/isa/pep10.hpp"
#include "core/sim/cores/cpu/pep_isa.hpp"
#include "core/sim/debugger/trace_device.hpp"
#include "core/sim/debugger/tvm_interpreter.hpp"
#include "core/sim/debugger/tvm_tracebuffer.hpp"
#include "core/sim/memory/ram/dense.hpp"
#include "core/sim/system.hpp"

namespace {
using M = isa::Pep10::Mnemonic;

// Offsets from a RAAA mnemonic's base opcode, mirroring how the ISA table lays the modes out. BR is not a RAAA
// instruction, but its base is likewise the immediate form, so `I` works for it too.
enum class Mode : u8 { i = 0, d = 1, n = 2, s = 3, sf = 4, x = 5 };
constexpr u8 op(M m, Mode mode = Mode::i) { return static_cast<u8>(static_cast<u8>(m) + static_cast<u8>(mode)); }

// No initiator and will be directed to system root.
const Operation app(Operation::Type::Application, Operation::Kind::data);

struct Harness {
  std::unique_ptr<System> sys;
  Dense *mem = nullptr;
  PepISA3CPU *cpu = nullptr;
  trace::BufferDevice *tbdev = nullptr;
};

Harness make_traced_cpu() {
  PepISA3CPU::Configuration cpu_cfg{
      Device::Configuration{.basename = "cpu", .compatible = PepISA3CPU::compatible}, PepISA3CPU::ISA::Pep10,
      "/memory"};
  System::Configuration root_cfg{{.basename = "/", .compatible = System::compatible}};
  Dense::Configuration mem_cfg{Device::Configuration{.basename = "memory", .compatible = Dense::compatible}, 0x00,
                               AddressSpan(0x0000, 0xffff)};
  trace::BufferDevice::Configuration tb_cfg{Device::Configuration{.basename = "trace"}, 4};

  Harness h;
  h.sys = std::make_unique<System>(root_cfg);
  h.mem = h.sys->make_device<Dense>(mem_cfg);
  h.cpu = h.sys->make_device<PepISA3CPU>(cpu_cfg, h.sys.get());
  h.tbdev = h.sys->make_device<trace::BufferDevice>(tb_cfg);
  // Binds a Recorder to every Traceable, including the register bank and CSRs the CPU builds for itself.
  h.sys->initialize();

  h.tbdev->trace(h.mem->id(), true);
  // Reaches the CPU's own initiator bit plus its register bank and CSRs.
  h.cpu->trace(true);
  return h;
}

// Load `program` at address 0, run `ticks` instructions, and report what the trace cost.
tvm::TraceBuffer::Footprint run(Harness &h, bits::span<const u8> program, int ticks) {
  h.mem->write(0, program, app);
  for (int i = 0; i < ticks; ++i) h.cpu->clock_tick(PulseSchedule::PulseIndex{0}, static_cast<u64>(i));
  return h.tbdev->buffer().footprint();
}

void report(const char *label, const tvm::TraceBuffer &tb, const tvm::TraceBuffer::Footprint &f) {
  SPDLOG_WARN("{}: {:.1f} B/instr over {} instrs (inlined: {:.1f}) | ratio {:.3f} | code {} templates {} data {} "
              "locations {} | {} templates promoted, {} hashes pending | {} KiB reserved",
              label, f.bytes_per_program(), f.programs, f.bytes_per_program_if_inlined(), f.compression_ratio(),
              f.code, f.templates, f.data, f.locations(), tb.template_count(), tb.pending_count(),
              tb.buffer_footprint() / 1024);
}
} // namespace

TEST_CASE("Trace footprint over a few thousand instructions",
          "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  constexpr int TICKS = 3000;

  // Best case for body de-duplication: the store always goes to the same address, so every instruction in the loop
  // encodes byte-identically each time round.
  //
  //   0x0000  LDWA 1,i        A = 1
  //   0x0003  ADDA 1,i        A += 1        <- loop
  //   0x0006  STWA 0x9000,d   mem[0x9000] = A
  //   0x0009  BR   0x0003
  const std::array<u8, 12> fixed_program{
      op(M::LDWA, Mode::i), 0x00, 0x01, //
      op(M::ADDA, Mode::i), 0x00, 0x01, //
      op(M::STWA, Mode::d), 0x90, 0x00, //
      op(M::BR, Mode::i),   0x00, 0x03, //
  };

  // Realistic case: the store walks through memory, so its body carries a different address every iteration and can
  // never match a previous one. The register-bank writes still de-duplicate, since those offsets are constant.
  //
  //   0x0000  LDWX 0,i        X = 0
  //   0x0003  ADDA 1,i        A += 1        <- loop
  //   0x0006  STWA 0x9000,x   mem[0x9000 + X] = A
  //   0x0009  ADDX 2,i        X += 2
  //   0x000C  BR   0x0003
  const std::array<u8, 15> walking_program{
      op(M::LDWX, Mode::i), 0x00, 0x00, //
      op(M::ADDA, Mode::i), 0x00, 0x01, //
      op(M::STWA, Mode::x), 0x90, 0x00, //
      op(M::ADDX, Mode::i), 0x00, 0x02, //
      op(M::BR, Mode::i),   0x00, 0x03, //
  };

  auto h_fixed = make_traced_cpu();
  const auto fixed = run(h_fixed, {fixed_program.data(), fixed_program.size()}, TICKS);

  auto h_walking = make_traced_cpu();
  const auto walking = run(h_walking, {walking_program.data(), walking_program.size()}, TICKS);

  report("fixed-address store ", h_fixed.tbdev->buffer(), fixed);
  report("walking-address store", h_walking.tbdev->buffer(), walking);

  // Every tick produced exactly one record, so the per-instruction numbers above mean what they say.
  CHECK(fixed.programs == TICKS);
  CHECK(walking.programs == TICKS);
  CHECK(fixed.programs == h_fixed.tbdev->instruction_count());
  CHECK(walking.programs == h_walking.tbdev->instruction_count());

  // Writes really were recorded -- an empty data chain would mean the CPU never opened a recording and the numbers
  // are measuring nothing.
  CHECK(fixed.data > 0);
  CHECK(walking.data > 0);

  // Both loops repeat register writes at constant offsets, so both find templates to share.
  CHECK(fixed.compression_ratio() > 1.0);
  // With the creation of SETMEMDX, the second access pattern remains friendly to deduplication.
  CHECK(walking.compression_ratio() > 2.0);
  // So there should only be a handful of instruction shapes, not one pending hash per store executed.
  CHECK(h_walking.tbdev->buffer().pending_count() < 32);
}

TEST_CASE("A recorded loop reverses back to the state it started from",
          "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  using R = isa::Pep10::Register;
  // The same fixed-address loop the footprint case measures, run short enough to reason about exactly. One setup
  // instruction, then ten times round the three-instruction body -- so A is carried across iterations and the store
  // overwrites the same address ten times. Undoing that needs every record replayed, in reverse, exactly once.
  constexpr int LOOPS = 10, TICKS = 1 + 3 * LOOPS;
  constexpr Address STORE = 0x9000;

  //   0x0000  LDWA 1,i        A = 1
  //   0x0003  ADDA 1,i        A += 1        <- loop
  //   0x0006  STWA 0x9000,d   mem[0x9000] = A
  //   0x0009  BR   0x0003
  const std::array<u8, 12> program{
      op(M::LDWA, Mode::i), 0x00, 0x01, //
      op(M::ADDA, Mode::i), 0x00, 0x01, //
      op(M::STWA, Mode::d), 0x90, 0x00, //
      op(M::BR, Mode::i),   0x00, 0x03, //
  };

  auto h = make_traced_cpu();
  h.mem->write(0, {program.data(), program.size()}, app);

  // Start from a state with nothing at its default. Zeroed registers would let a broken undo pass by simply leaving
  // everything where a fresh machine already sits, and a zeroed store address would hide a store that never got
  // reverted. These writes happen outside any clock_tick, so no recording is open and none of them are traced --
  // they are the baseline, not part of the history being undone.
  h.cpu->write_register_uncached(R::A, 0xAAAA);
  h.cpu->write_register_uncached(R::X, 0xBBBB);
  h.cpu->write_register_uncached(R::SP, 0xCCCC);
  h.cpu->write_register_uncached(R::PC, 0x0000);
  h.cpu->write_packed_csr(0b1111);
  ((Target *)h.mem)->write<u16, bits::host_is_le>(STORE, 0xDEAD, app);

  struct Snapshot {
    u16 a, x, sp, pc, stored;
    u8 csr;
    bool operator==(const Snapshot &) const = default;
  };
  // Read the register bank rather than PepISA3CPU::read_register, which serves PC out of the _pc shadow. The trace
  // records bank writes, so the bank is what undo restores; the shadow is reloaded from it at the top of the next
  // clock_tick, which is why undoing leaves the CPU runnable rather than merely consistent-looking.
  auto capture = [&] {
    return Snapshot{h.cpu->read_register_uncached(R::A),
                    h.cpu->read_register_uncached(R::X),
                    h.cpu->read_register_uncached(R::SP),
                    h.cpu->read_register_uncached(R::PC),
                    ((Target *)h.mem)->read<u16, bits::host_is_le>(STORE, app).second,
                    h.cpu->read_packed_csr()};
  };

  const auto initial = capture();
  const auto before = h.tbdev->buffer().cursor();

  for (int i = 0; i < TICKS; ++i) h.cpu->clock_tick(PulseSchedule::PulseIndex{0}, static_cast<u64>(i));

  const auto after = capture();
  const auto end = h.tbdev->buffer().cursor();
  REQUIRE(h.tbdev->instruction_count() == TICKS);

  // Same shape of report as the measurement case above. Worth printing even though this test is about correctness:
  // these are the bytes undo has to walk back through, and a body that stops templatizing shows up here first.
  const auto before_undo = h.tbdev->buffer().footprint();
  report("reversible loop", h.tbdev->buffer(), before_undo);

  // The loop actually ran, and ran the way the program says: LDWA sets A to 1, each iteration adds one, and the store
  // follows A. Asserting the value rather than just "something changed" is what makes the undo check below mean
  // something -- otherwise a CPU that did nothing would also "revert" perfectly.
  CHECK(after.a == 1 + LOOPS);
  CHECK(after.stored == 1 + LOOPS);
  CHECK(after.pc == 0x0003); // parked at the top of the loop body
  CHECK(after.a != initial.a);
  CHECK(after.stored != initial.stored);
  // X and SP are never written, so they are the part of the state undo is not responsible for.
  CHECK(after.x == initial.x);
  CHECK(after.sp == initial.sp);

  auto blaster = h.sys->make_trace_interpreter();
  // Load-bearing, and not only for INVCALL arm selection. Each record states the access the CPU actually made, so
  // that a trace can be analyzed; replaying that access verbatim would file the undo as new history and re-trigger
  // any memory-mapped side effect it had. Going backwards is what tells SET* to substitute a BufferInternal access
  // instead -- see Backend::effective_access. Drop this line and the footprint below doubles.
  blaster->backend().set_direction(tvm::Direction::Backward);

  // Reverse order, one program at a time. Each record XORs the bytes it changed back to what they were, so the
  // sequence only lands on `initial` if every record is replayed exactly once and in the right order -- a skipped or
  // repeated record leaves its own writes inverted.
  auto recorded = h.tbdev->buffer().range(before, end);
  int undone = 0;
  for (auto it = recorded.end(); it != recorded.begin();) {
    --it;
    blaster->run(*it);
    REQUIRE(blaster->stopped());
    REQUIRE(blaster->csrs().F == 0);
    ++undone;
  }
  CHECK(undone == TICKS);

  CHECK(capture() == initial);

  // Undoing wrote to every target the loop touched and cost the buffer nothing. There is no second report because
  // there should be nothing to compare: going backwards makes SET* substitute a BufferInternal access, so the writes
  // never re-enter the recorder. Without that, stepping backwards grows the very trace it is walking.
  //
  // Checked field by field as well as in total, because the failure this catches lands in exactly one of them --
  // `data` doubles while code, templates and programs hold, since payload is appended without any commit().
  const auto after_undo = h.tbdev->buffer().footprint();
  CHECK(after_undo.total() == before_undo.total());
  CHECK(after_undo.data == before_undo.data);
  CHECK(after_undo.code == before_undo.code);
  CHECK(after_undo.code_if_inlined == before_undo.code_if_inlined);
  CHECK(after_undo.templates == before_undo.templates);
  CHECK(after_undo.programs == before_undo.programs);
  // Nothing was committed either, so the history is the same length and ends where it did.
  CHECK(h.tbdev->instruction_count() == TICKS);
  CHECK(h.tbdev->buffer().cursor() == end);
}
