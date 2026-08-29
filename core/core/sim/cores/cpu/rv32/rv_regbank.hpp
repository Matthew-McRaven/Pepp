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
#include <array>
#include "core/arch/riscv/isa/rv_base.hpp"
#include "core/sim/api/device.hpp"
#include "core/sim/api/memory.hpp"
#include "core/sim/api/trace.hpp"
#include "core/sim/debugger/register_scanner.hpp"
#include "core/sim/debugger/trace_recorder.hpp"

// RISC-V 32-bit register file, held as aligned C++ members
class RV32RegisterBank final : public Device, public Target, public Traceable {
public:
  using Register = riscv::XReg;
  static constexpr u8 REGISTER_COUNT = 32;
  // Each register occupies four bytes of the Target address space.
  static constexpr u8 REGISTER_BYTES = 4;
  // Fully synthetic w/o local config values.
  struct Configuration : public Device::Configuration {};

  explicit RV32RegisterBank(Configuration config);
  ~RV32RegisterBank() = default;
  RV32RegisterBank(const RV32RegisterBank &) = delete;
  RV32RegisterBank &operator=(const RV32RegisterBank &) = delete;

  template <Register R> u32 read() const {
    static_assert(static_cast<u8>(R) < REGISTER_COUNT, "not a general-purpose register");
    return _regs[static_cast<u8>(R)];
  }
  template <Register R> void write(u32 value) {
    static_assert(static_cast<u8>(R) < REGISTER_COUNT, "not a general-purpose register");
    // Writes to x0 are discarded rather than refused; that is the architectural behaviour.
    if constexpr (R != Register::x0) store(R, value, _op);
  }

  u32 read(Register reg) const { return _regs[static_cast<u8>(reg)]; }
  void write(Register reg, u32 value) {
    if (reg == Register::x0) return;
    store(reg, value, _op);
  }

  // The program counter is architectural state but not a general-purpose register.
  u32 read_pc() const { return _pc; }
  void write_pc(u32 value) { store_pc(value, _op); }
  // Store PC without recording a trace, so the CPU can emit one specialized increment per
  // instruction instead of a full register write.
  void write_pc_untraced(u32 value) { _pc = value; }

  // The scan reference for a register, so a CPU can name it when recording something this class does not emit.
  RegisterScan::RegisterRef ref(Register reg) const;
  RegisterScan::RegisterRef pc_ref() const;

  // Device interface
  void initialize(System *) override;
  void reset() override;
  const Device::Configuration &config() const override;
  const Configuration &casted_config() const;
  const Device::ID id() const override;
  Device::Type type() const override;
  // Throws. See the definition.
  std::unique_ptr<DeviceSerializer> serializer() const override;

  // Traceable interface
  void set_recorder(const trace::Recorder &recorder) override;
  bool can_generate_traces() const override;
  void trace(bool enabled) override;
  bool traced() const override;
  void on_traced_changed(bool enabled) override;

  // Target interface, providing little-endian access to the regs as an array-of-bytes.
  AddressSpan span() const override;
  Result read(Address address, bits::span<u8> dest, Operation op) const override;
  Result write(Address address, bits::span<const u8> src, Operation op) override;
  void clear(u8 fill) override;
  void dump(bits::span<u8> dest) const override;
  void collect_changes(pepp::core::IntervalSet<Address> &changed) const override;
  void clear_changes() override;

  // ID of the CPU which owns this register bank, which is needed to emit traces to the correct temporary buffer when
  // not using the Target API.
  void set_initiator(Device::ID cpu);

private:
  // Record a trace and then store the value in-place.
  void store(Register reg, u32 value, Operation op) {
    const auto index = static_cast<u8>(reg);
    // Cheap to compute new ^ old in a single instruction rather than construct a lambda which does the same thing.
    if (_traced) _trace.emit_write_register(op, _refs[index], _regs[index] ^ value);
    _regs[index] = value;
  }
  void store_pc(u32 value, Operation op) {
    if (_traced) _trace.emit_write_register(op, _pc_ref, _pc ^ value);
    _pc = value;
  }

  Configuration _config;
  std::array<u32, REGISTER_COUNT> _regs{};
  u32 _pc = 0;

  // Filled in by initialize(), and used to emit traces.
  std::array<RegisterScan::RegisterRef, REGISTER_COUNT> _refs{};
  RegisterScan::RegisterRef _pc_ref{};
  trace::Recorder _trace;
  // Cached value of TraeBuffer::traced(this->id()) to avoid repeated lookups in our hot path.
  bool _traced = false;
  Operation _op = Operation(Operation::Type::Standard, Operation::Kind::data, Device::ID{0});
};
