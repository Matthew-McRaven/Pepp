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
#include <tuple>
#include "core/arch/pep/isa/pep10.hpp"
#include "core/sim/api/device.hpp"
#include "core/sim/api/memory.hpp"
#include "core/sim/api/trace.hpp"
#include "core/sim/debugger/register_scanner.hpp"
#include "core/sim/debugger/trace_recorder.hpp"

// The Pep status flags, held as one packed byte. NZVC occupies the low nibble of a single u8, N at bit 3 down to C at
// bit 0. Still a Target over the same one-byte address space, presenting the same value, so address-based callers keep
// working.
class PepCSRBank final : public Device, public Target, public Traceable {
public:
  using CSR = isa::Pep10::CSR;
  // Fully synthetic: the CPU constructs it, nothing parses one, and it is never written out.
  struct Configuration : public Device::Configuration {};

  explicit PepCSRBank(Configuration config);
  ~PepCSRBank() = default;
  PepCSRBank(const PepCSRBank &) = delete;
  PepCSRBank &operator=(const PepCSRBank &) = delete;

  // --- Typed access.
  bool read_n() const { return (_nzvc & bit(CSR::N)) != 0; }
  bool read_z() const { return (_nzvc & bit(CSR::Z)) != 0; }
  bool read_v() const { return (_nzvc & bit(CSR::V)) != 0; }
  bool read_c() const { return (_nzvc & bit(CSR::C)) != 0; }

  void write_n(bool value);
  void write_z(bool value);
  void write_v(bool value);
  void write_c(bool value);

  // All four at once, which is what an ALU result sets. One record rather than four.
  u8 read_packed() const { return _nzvc; }
  void write_packed(u8 value);

  // --- Enum dispatch, for callers holding a CSR rather than a name. ---
  bool read(CSR csr) const { return (_nzvc & bit(csr)) != 0; }
  void write(CSR csr, bool value);
  // The scan reference for the packed register. A field of 0 addresses the whole byte; the individual flags are
  // exposed as fields of it.
  RegisterScan::RegisterRef ref() const { return _ref; }

  // The flags run N, Z, V, C and the packing puts N highest, so a flag's bit counts down from the top of the nibble.
  static constexpr u8 bit(CSR csr) { return static_cast<u8>(1 << (3 - static_cast<u8>(csr))); }
  static constexpr u8 MASK = 0x0F;

  static constexpr u8 pack(bool n, bool z, bool v, bool c) {
    return static_cast<u8>((n ? bit(CSR::N) : 0) | (z ? bit(CSR::Z) : 0) | (v ? bit(CSR::V) : 0) |
                           (c ? bit(CSR::C) : 0));
  }
  static constexpr std::tuple<bool, bool, bool, bool> unpack(u8 nzvc) {
    return {(nzvc & bit(CSR::N)) != 0, (nzvc & bit(CSR::Z)) != 0, (nzvc & bit(CSR::V)) != 0,
            (nzvc & bit(CSR::C)) != 0};
  }

  // Device interface
  void initialize(System *) override;
  void reset() override;
  const Device::Configuration &config() const override;
  const Configuration &casted_config() const;
  const Device::ID id() const override;
  Device::Type type() const override;
  std::unique_ptr<DeviceSerializer> serializer() const override;

  // Traceable interface
  void set_recorder(const trace::Recorder &recorder) override;
  bool can_generate_traces() const override;
  void trace(bool enabled) override;
  bool traced() const override;
  void on_traced_changed(bool enabled) override;

  // Target interface. The slow, generic path, over the same single byte the Dense covered.
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
  // Record the xor if traced, then store. Always a whole-byte record: a single flag is a field of the packed
  // register, and SETREGX names the register rather than the field.
  void store(u8 value, Operation op);

  Configuration _config;
  u8 _nzvc = 0;
  // Filled in by initialize(). SETREGX names its target by scan id, so a write cannot be recorded without it.
  RegisterScan::RegisterRef _ref{};
  trace::Recorder _trace;
  // Mirror of the buffer's traced bit, pushed via on_traced_changed(). Read on every write.
  bool _traced = false;
  Operation _op = Operation(Operation::Type::Standard, Operation::Kind::data, Device::ID{0});
};
