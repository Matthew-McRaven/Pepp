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
#include <vector>
#include "core/sim/api/device.hpp"
#include "core/sim/api/event.hpp"
#include "core/sim/api/memory.hpp"
#include "core/sim/api/trace.hpp"
#include "core/sim/debugger/trace_recorder.hpp"

// A memory-mapped register holding only its current value, e.g., Pep/10's power-off port.
// Program writes raise MemoryWritten so that other devices (e.g., clocks) can react to them.
class StateRegister final : public Device, public Target, public Traceable, public EventSource {
public:
  static const inline std::string compatible = "io,state";
  struct Configuration : public Device::Configuration {
    u8 fill{0};
    AddressSpan span{};
  };
  StateRegister(Configuration config);
  ~StateRegister() = default;
  StateRegister(const StateRegister &) = delete;
  StateRegister &operator=(const StateRegister &) = delete;
  bool changed() const;

  // Device interface
  void reset() override;
  const Device::Configuration &config() const override;
  const Configuration &casted_config() const;
  const Device::ID id() const override;
  Device::Type type() const override;
  std::unique_ptr<DeviceSerializer> serializer() const override;
  static std::unique_ptr<DeviceSerializer> make_serializer();

  // Traceable interface
  void set_recorder(const trace::Recorder &recorder) override;
  bool can_generate_traces() const override;
  void trace(bool enabled) override;
  bool traced() const override;

  // Target interface
  AddressSpan span() const override;
  Result read(Address address, bits::span<u8> dest, Operation op) const override;
  // Only Standard writes raise MemoryWritten. The event is raised after the value is updated.
  Result write(Address address, bits::span<const u8> src, Operation op) override;
  void clear(u8 fill) override;
  void dump(bits::span<u8> dest) const override;
  void collect_changes(pepp::core::IntervalSet<Address> &changed) const override;
  void clear_changes() override;

  using Target::read;
  using Target::write;

private:
  Configuration _config;
  std::vector<u8> _data;
  bool _changed = false;
  trace::Recorder _trace;
};
