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
#include "core/arch/pep/isa/pep10.hpp"
#include "core/sim/api/device.hpp"
#include "core/sim/api/memory.hpp"
#include "core/sim/api/trace.hpp"
#include "core/sim/debugger/register_scanner.hpp"
#include "core/sim/debugger/trace_recorder.hpp"

// The Pep register file, held as C++ membner variables rather than offsets into an array.
// It replaces the previous Dense* Target. With some optimization, the compiler is better able to elide useless calls to
// memcpy. Registers are stored in host order for the direct RegisterBank API, but exposed in LE order via Target.
// From the Target API, register N occupies bytes [N*2, N*2+1]
//
// Pep8, Pep9, amd Pep10 declare identical Register enums, so one bank serves all ISAs.
class PepRegisterBank final : public Device, public Target, public Traceable {
public:
  using Register = isa::Pep10::Register;
  static constexpr auto REGISTER_COUNT = static_cast<u8>(Register::INVALID);
  // Fully synthetic: the CPU constructs it, nothing parses one, and it is never written out.
  struct Configuration : public Device::Configuration {};

  explicit PepRegisterBank(Configuration config);
  ~PepRegisterBank() = default;
  PepRegisterBank(const PepRegisterBank &) = delete;
  PepRegisterBank &operator=(const PepRegisterBank &) = delete;

  // Inline, compile-time form which the compiler reduces to a register read or write, usable when you know the target
  // register at compile time.
  template <Register R> auto read() const {
    if constexpr (R == Register::A) return _a;
    else if constexpr (R == Register::X) return _x;
    else if constexpr (R == Register::SP) return _sp;
    else if constexpr (R == Register::PC) return _pc;
    else if constexpr (R == Register::IS) return _is;
    else if constexpr (R == Register::OS) return _os;
    else static_assert(false, "no storage backs that register");
  }
  template <Register R> void write(u16 value) {
    if constexpr (R == Register::IS) store(R, _is, static_cast<u8>(value), _op);
    else if constexpr (R == Register::A) store(R, _a, value, _op);
    else if constexpr (R == Register::X) store(R, _x, value, _op);
    else if constexpr (R == Register::SP) store(R, _sp, value, _op);
    else if constexpr (R == Register::PC) store(R, _pc, value, _op);
    else if constexpr (R == Register::OS) store(R, _os, value, _op);
    else static_assert(false, "no storage backs that register");
  }

  // Variants used at design-time when you know which register you want to use. Syntactic sugar over read<R> and
  // write<R>
  u16 read_a() const { return read<Register::A>(); }
  u16 read_x() const { return read<Register::X>(); }
  u16 read_sp() const { return read<Register::SP>(); }
  u16 read_pc() const { return read<Register::PC>(); }
  u16 read_os() const { return read<Register::OS>(); }
  // One byte, and stored as a single u8 to avoid masking a u16 to one byte on each operation.
  u8 read_is() const { return read<Register::IS>(); }

  void write_a(u16 value);
  void write_x(u16 value);
  void write_sp(u16 value);
  void write_pc(u16 value);
  void write_os(u16 value);
  void write_is(u8 value);

  // Store PC without recording a trace. Used to implement the specialized PC increment in conjuction with the CPU's
  // clock_tick handler.
  void write_pc_untraced(u16 value) { _pc = value; }

  // Inlining reduce cost of dynamic read to a jumptable rather than JT + call.
  u16 read(Register reg) const {
    switch (reg) {
    case Register::A: return read<Register::A>();
    case Register::X: return read<Register::X>();
    case Register::SP: return read<Register::SP>();
    case Register::PC: return read<Register::PC>();
    case Register::IS: return read<Register::IS>();
    case Register::OS: return read<Register::OS>();
    default: return 0;
    }
  }
  void write(Register reg, u16 value) {
    switch (reg) {
    case Register::A: write<Register::A>(value); break;
    case Register::X: write<Register::X>(value); break;
    case Register::SP: write<Register::SP>(value); break;
    case Register::PC: write<Register::PC>(value); break;
    case Register::IS: write<Register::IS>(value); break;
    case Register::OS: write<Register::OS>(value); break;
    default: break;
    }
  }
  // The scan reference for a register, so a CPU can name it when recording something this class does not emit.
  RegisterScan::RegisterRef ref(Register reg) const;

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

  // Target interface, providing big-endian access to the registers as-if they were a contiguous array of bytes.
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
  template <std::integral I> void store(Register reg, I &slot, I value, Operation op) {
    // Cheap to compute new ^ old in a single instruction rather than construct a lambda which does the same thing.
    if (_traced) _trace.emit_write_register(op, _refs[static_cast<u8>(reg)], static_cast<I>(slot ^ value));
    slot = value;
  }
  void write_slot(Register reg, u16 value, Operation op);

  Configuration _config;
  // Avoid byte-swapping in the non-Target case by using real integers rather than an array of bytes.
  u16 _a = 0, _x = 0, _sp = 0, _pc = 0, _os = 0;
  u8 _is = 0;
  // Filled in by initialize(), and used to emit traces.
  std::array<RegisterScan::RegisterRef, REGISTER_COUNT> _refs{};
  trace::Recorder _trace;
  // Cached value of TraeBuffer::traced(this->id()) to avoid repeated lookups in our hot path.
  bool _traced = false;
  Operation _op = Operation(Operation::Type::Standard, Operation::Kind::data, Device::ID{0});
};
