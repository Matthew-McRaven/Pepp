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
#include <memory>
#include <string>
#include "core/ds/opaque_handle.hpp"
#include "core/integers.h"
#include "core/math/bitmanip/enums.hpp"

// An object with heavy dependencies on nlohmann/json
// Will will only operate on pointers to it in our headers, and implementing its API in CPP
// does require pullin in json header. See systemparser.hpp for details.
struct DeviceSerializer;
class System;

std::string child_name(std::string_view parent_fullname, std::string_view child_basename);
struct Device {
  using ID = pepp::OpaqueHandle<struct DeviceID, u8>;
  using IDGenerator = std::function<Device::ID()>;
  // Do not parse or serialize: id, fullname, or skip_serialize.
  // They are inferred at device creation time
  struct Configuration {
    Device::ID id;
    std::string basename, compatible;
    std::string fullname;
    // If true, this device will not be serialized. This is useful when one device spawns child devices which were not
    // part of the original parse tree. For example, the register banks and CSRs of a Pep/10 CPU are created during CPU
    // construction.
    bool skip_serialize = false;
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
    MemoryInitiator = MemoryTarget << 1,
    ClockSource = MemoryInitiator << 1,
    ClockSink = ClockSource << 1,
    // Synthetic devices, which are not part of the original device tree but are created by the simulator to allow
    // access to portions of the simulation
    TraceBuffer = ClockSink << 1,
    Traceable = TraceBuffer << 1,
    // Keep the synthetic system root at the end of the list by convention.
    SystemRoot = Traceable << 1,
    MASK = (SystemRoot << 1) - 1,
  };

  virtual ~Device() = default;
  // Some devices need further initialization after the full device tree has been constructed.
  // Classes which require this 2nd stage of init should override this method.
  // As part of this initialize step, you should expose all registers to the Systems RegisterScan.
  virtual void initialize(System *) {}
  // Return this device's own state to what it held immediately after initialization. This includes clearing performance
  // counters in addition to a Target's memory. Does not recurse into child devices.
  virtual void reset() = 0;
  virtual const Configuration &config() const = 0;
  virtual const Device::ID id() const = 0;
  // Helper to test if this device implements a particular interface type.
  virtual Device::Type type() const { return Type::None; }
  // Features specific to the concrete  instance of the device.
  virtual u64 features() const { return 0; }
  // Return a ptr to a type which can convert this object to/from JSON.
  virtual std::unique_ptr<DeviceSerializer> serializer() const = 0;
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
};
consteval void is_bitflags(Device::Type);

template <> struct std::hash<Device::ID> {
  std::size_t operator()(const Device::ID &v) const noexcept {
    return std::hash<Device::ID::underlying_type>{}(v.value);
  }
};

// Can't be inside class def because is_bitflags is not yet visible.
inline Device *Device::capability(Type t) {
  using namespace bits;
  if (any(type() & t)) return this;
  else return nullptr;
}