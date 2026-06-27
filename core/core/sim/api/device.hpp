/*
 * Copyright (c) 2024-2026 J. Stanley Warford, Matthew McRaven
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

#include <functional>
#include <string>
#include "core/ds/opaque_handle.hpp"
#include "core/integers.h"
#include "core/math/bitmanip/enums.hpp"

struct Device {
  using ID = pepp::OpaqueHandle<struct DeviceID, u8>;
  using IDGenerator = std::function<Device::ID()>;
  struct Descriptor {
    ID id = Device::ID{0};
    std::string basename = "", fullname = "", compatible = "";
    std::string child_name(std::string_view child_basename) const;
  };
  // Bitflags telling you what interfaces this abstract device implements.
  // e.g., if any(type() & Type::MemoryTarget), then this device implements the MemoryTarget interface.
  // You could then get a pointer to the interface by calling capability(Type::MemoryTarget).
  // It is a bitmask, allowing a device to implement multiple interfaces.
  enum class Type : u64 {
    None = 0,
    Root = 1,
    // Generic device types
    MemoryTarget = Root << 1,
    MemoryTranslator = MemoryTarget << 1,
    MemoryInitiator = MemoryTranslator << 1,
    ClockSource = MemoryInitiator << 1,
    ClockSink = ClockSource << 1,
    // Synthetic devices, which are not part of the original device tree but are created by the simulator to allow
    // access to portions of the simulation
    TraceBuffer = ClockSink << 1,
    // Keep the synthetic system root at the end of the list by convention.
    SystemRoot = TraceBuffer << 1,
    MASK = (SystemRoot << 1) - 1,
  };

  Device(Descriptor desc) : _desc(desc) {}
  virtual ~Device() = default;
  const Descriptor &descriptor() const { return _desc; }
  // Helper to test if this device implements a particular interface type.
  virtual Type type() const { return Type::None; }
  // Features specific to the concrete  instance of the device.
  virtual u64 features() const { return 0; }
  // Given one of the interface types, return an instance of that interface if this device implements it, otherwise
  // return nullptr.
  template <typename Concrete> Concrete *capability() {
    using namespace bits;
    if (!any(type() & Concrete::TypeMask)) return nullptr;
    Device *p = capability(Concrete::TypeMask);
    return dynamic_cast<Concrete *>(p);
  }

protected:
  // Subclasses should override this to return a pointer to the appropriate interface if the requested type is
  // supported, otherwise nullptr.
  virtual Device *capability(Device::Type t);

private:
  Descriptor _desc;
};
consteval void is_bitflags(Device::Type);
