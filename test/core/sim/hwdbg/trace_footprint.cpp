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
  SPDLOG_WARN("{}: {:.1f} B/instr over {} instrs (inlined: {:.1f}) | ratio {:.3f} | code {} templates {} data {} | "
              "{} templates promoted, {} hashes pending | {} KiB reserved",
              label, f.bytes_per_program(), f.programs, f.bytes_per_program_if_inlined(), f.compression_ratio(),
              f.code, f.templates, f.data, tb.template_count(), tb.pending_count(),
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
  CHECK(walking.compression_ratio() > 1.0);

  // The whole point of running two loops: the only difference between them is whether the store's target address is
  // constant, and that alone decides whether the store's body can ever be shared. Until the address moves out of the
  // body and into the payload, ordinary memory traffic gets the worse of these two numbers.
  CHECK(walking.compression_ratio() < fixed.compression_ratio());

  // And the cost shows up twice: the walking store not only fails to share, it leaves a hash behind on every
  // iteration for a body that will never be seen again.
  CHECK(h_walking.tbdev->buffer().pending_count() > h_fixed.tbdev->buffer().pending_count());
}
