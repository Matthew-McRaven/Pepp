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
#pragma once

#include "core/arch/riscv/isa/rv_instruction.hpp"
#include "core/arch/riscv/isa/rv_instruction_list.hpp"
#include "core/sim/api/clock.hpp"
#include "core/sim/api/device.hpp"
#include "core/sim/api/memory.hpp"
#include "core/sim/api/trace.hpp"
#include "core/sim/cores/cpu/rv32/rv_regbank.hpp"
#include "core/sim/debugger/register_scanner.hpp"
#include "core/sim/debugger/trace_recorder.hpp"

// RISC-V 32-bit core.
class RV32CPU final : public Device, public ClockSink, public Traceable, public Initiator {
public:
  static const inline std::string compatible = "cpu,riscv,rv32";
  using Register = riscv::XReg;

  struct Configuration : public Device::Configuration {
    // Name of device to use as the target for memory access. Resolved to Target* during initialize().
    std::string target;
  };
  RV32CPU(Configuration cfg, System *sys);
  ~RV32CPU() = default;
  RV32CPU(RV32CPU &&other) noexcept = default;
  RV32CPU &operator=(RV32CPU &&other) = default;
  RV32CPU(const RV32CPU &) = delete;
  RV32CPU &operator=(const RV32CPU &) = delete;

  // Explicitly inlined: called several times per simulated instruction.
  inline const Target *target() const { return _target; }
  inline Target *target() { return _target; }

  // Device interface
  void initialize(System *) override;
  void reset() override;
  const Device::Configuration &config() const override;
  const Configuration &casted_config() const;
  const Device::ID id() const override;
  Device::Type type() const override;
  std::unique_ptr<DeviceSerializer> serializer() const override;
  static std::unique_ptr<DeviceSerializer> make_serializer();

  // ClockSink interface
  void clock_tick(PulseSchedule::PulseIndex idx, u64 tick) override;
  void set_clock_source(const ClockSource *src) override;
  const ClockSource *clock_source() const override;

  // Traceable interface
  void set_recorder(const trace::Recorder &recorder) override;
  bool can_generate_traces() const override;
  bool traced() const override;
  void on_traced_changed(bool enabled) override;
  void trace(bool enabled) override;

  // Register file. rd/rs1/rs2 come out of the decoded word, so these take a runtime register;
  // the bank's compile-time form is for the rare site where the ISA fixes the register.
  u32 read_register(Register reg) const;
  void write_register(Register reg, u32 value);

  RV32RegisterBank *registers() const { return _regbank; }

  // While an instruction is in flight this holds the working PC, written back once at the end
  // of clock_tick so a straight-line instruction costs one register update instead of several.
  u32 read_pc() const { return _pc; }
  void write_pc(u32 value) { _pc = value; }

  Operation op_data() const { return _op_data; }
  Operation op_fetch() const { return _op_fetch; }

private:
  struct PerfCounters {
    u64 instructions = 0;
  } _count = {};
  // Only meaningful between the start and end of clock_tick. See read_pc().
  u32 _pc = 0;
  Configuration _config;
  trace::Recorder _trace;
  RV32RegisterBank *_regbank = nullptr;
  Target *_target = nullptr;
  // Mirror of the buffer's traced bit, pushed by TraceBuffer::trace, so the hooks below cost a
  // predictable branch rather than a call when tracing is off.
  bool _may_trace = true;
  // Override once our id is known.
  Operation _op_data = Operation(Operation::Type::Standard, Operation::Kind::data, Device::ID{0});
  Operation _op_fetch = Operation(Operation::Type::Standard, Operation::Kind::instruction, Device::ID{0});
  const ClockSource *_clk = nullptr;

  // Fetch the word at _pc. Reads four bytes; a compressed-instruction fetch will need to look
  // at the low half first to decide how much to consume.
  riscv::rv_instruction2 fetch();
  void handle(RvOp op, riscv::rv_instruction2 w);
};
