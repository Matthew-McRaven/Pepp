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
// memcpy. Registers are stored in host order for the direct RegisterBank API, but exposed in LE order via Target. This
// maintains backwards-compatibility with usage of the Dense* register bank.
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

  // --- Typed access.
  u16 read_a() const { return _a; }
  u16 read_x() const { return _x; }
  u16 read_sp() const { return _sp; }
  u16 read_pc() const { return _pc; }
  u16 read_os() const { return _os; }
  // One byte, and stored as a single u8 to avoid masking a u16 to one byte on each operation.
  u8 read_is() const { return _is; }

  void write_a(u16 value);
  void write_x(u16 value);
  void write_sp(u16 value);
  void write_pc(u16 value);
  void write_os(u16 value);
  void write_is(u8 value);

  // Store PC without recording. The CPU defers PC writeback to the end of an instruction and records the change
  // itself as a STEPREG, which is smaller than a SETREGX when the delta is the same every time -- so this must not
  // emit one as well. The caller owns the record; this owns the value.
  void write_pc_untraced(u16 value) { _pc = value; }

  // --- Enum dispatch, for callers holding a Register rather than a name. Switches to the above. ---
  u16 read(Register reg) const;
  void write(Register reg, u16 value);
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

  // Target interface. The slow, generic path: big-endian, bounds checked, byte addressable. Keeps
  // backwards-compatibility with previus tests / usages.
  using Target::read;
  using Target::write;
  AddressSpan span() const override;
  Result read(Address address, bits::span<u8> dest, Operation op) const override;
  Result write(Address address, bits::span<const u8> src, Operation op) override;
  void clear(u8 fill) override;
  void dump(bits::span<u8> dest) const override;

  // The Operation the typed writers record under. Set once, by the CPU that owns this bank.
  void set_initiator(Device::ID cpu);

private:
  // Common tail of every typed write: record the xor if traced, then store. Templated so the recorded packet is the
  // register's own width rather than a fixed word.
  template <std::integral I> void store(Register reg, I &slot, I value, Operation op);
  // Enum dispatch for the Target path, which has a value and a slot index but no name.
  void write_slot(Register reg, u16 value, Operation op);

  Configuration _config;
  // Host order, always. Nothing here is ever byte swapped in place; the Target path converts on the way out.
  u16 _a = 0, _x = 0, _sp = 0, _pc = 0, _os = 0;
  u8 _is = 0;
  // Filled in by initialize(). SETREGX names its target by scan id, so a write cannot be recorded without these.
  std::array<RegisterScan::RegisterRef, REGISTER_COUNT> _refs{};
  trace::Recorder _trace;
  // Mirror of the buffer's traced bit, pushed via on_traced_changed(). Read on every write.
  bool _traced = false;
  Operation _op = Operation(Operation::Type::Standard, Operation::Kind::data, Device::ID{0});
};
