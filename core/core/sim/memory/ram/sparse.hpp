/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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
#include "core/ds/alloc/paged.hpp"
#include "core/sim/api/device.hpp"
#include "core/sim/api/memory.hpp"
#include "core/sim/api/trace.hpp"
#include "core/sim/debugger/trace_recorder.hpp"

class Sparse final : public Device, public Target, public Traceable {
public:
  static const inline std::string compatible = "ram,sparse";
  struct Configuration : public Device::Configuration {
    u8 fill = 0;
    AddressSpan span{};
  };
  Sparse(Configuration config);
  ~Sparse() = default;
  Sparse(Sparse &&other) noexcept = default;
  Sparse &operator=(Sparse &&other) = default;
  // Disable copy construction and assignment, since it would be incorrect for
  // multiple objects to share a device descriptor.
  Sparse(const Sparse &) = delete;
  Sparse &operator=(const Sparse &) = delete;

  // Device interface
  const Device::Configuration &config() const override;
  const Configuration &casted_config() const;
  const Device::ID id() const override;
  Device::Type type() const override;
  u64 features() const override;
  std::unique_ptr<DeviceSerializer> serializer() const override;
  static std::unique_ptr<DeviceSerializer> make_serializer();

  // TraceSource interface
  void set_recorder(const trace::Recorder &recorder) override;
  bool can_generate_traces() const override;
  void trace(bool enabled) override;
  bool traced() const override;

  // Target interface
  AddressSpan span() const override;
  Result read(Address address, bits::span<u8> dest, Operation op) const override;
  Result write(Address address, bits::span<const u8> src, Operation op) override;
  void clear(u8 fill) override;
  void dump(bits::span<u8> dest) const override;

private:
  Configuration _config;
  pepp::bts::PagedPool<u8> _pool;
  trace::Recorder _trace;
};
