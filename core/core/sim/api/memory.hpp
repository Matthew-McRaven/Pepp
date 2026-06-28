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

#include <string>
#include "core/integers.h"
#include "core/math/bitmanip/span.hpp"
#include "core/math/bitmanip/swap.hpp"
#include "core/math/geom/interval.hpp"
#include "core/sim/api/device.hpp"

using Tick = u32;
using Address = u32;
using AddressSpan = pepp::core::Interval<Address>;

// If select memory operations fail (e.g., lack of MMI, unmapped address in
// bus), specify the behavior of the target.
enum class FailPolicy {
  YieldDefaultValue, // The target picks some arbitrary default value, and
  // returns it successfully.
  RaiseError // The target returns an appropriate error message.
};

// In API v1, there was a concept of effectful and speculative.
// effectful=false implied that the memory operation was triggered by the UI.
// speculative=true implied some form of memory pre-fetch triggered by the
// simulated hardware. In no case was effectful=false, speculative=true a
// meaningful combination.
//
// In API v2, these have been condensed into a Operation::Type, eliminating the
// impossible case.
struct Operation {
  enum class Type : u8 {
    // Access triggered by the application / user interface.
    // Should not trigger memory-mapped IO, cache misses, etc.
    Application = 1,
    // Speculative access triggered within the simulation. Probably shouldn't
    // trigger
    // MMIO, but this is hardware dependent.
    Speculative = 2,
    // Access triggered by the simulator while performing some analysis
    // operation.
    // It must never trigger memory-mapped IO nor is it allowed to emit trace
    // events.
    BufferInternal = 3,
    // Non-speculative access triggered within the simulation. Should trigger
    // memory mapped IO,
    // cache updates, etc.
    Standard = 0,
  } type = Type::Standard;
  enum class Kind : bool { instruction = false, data = true } kind;
};

struct Target {
  struct Result {
    // Number of simulation ticks required to complete the memory op.
    // 0 indicates the operation completed immediately.
    Tick delay;
    // Did the memory access trigger a breakpoint?
    bool pause;
  };

  static constexpr Device::Type TypeMask = Device::Type::MemoryTarget;
  virtual ~Target() = default;

  virtual AddressSpan span() const = 0;
  virtual Result read(Address address, bits::span<u8> dest, Operation op) const = 0;
  virtual Result write(Address address, bits::span<const u8> src, Operation op) = 0;
  virtual void clear(u8 fill) = 0;

  // If dest is larger than maxOffset-minOffset+1, copy bytes from this target
  // to the span.
  virtual void dump(bits::span<u8> dest) const = 0;

  // These convenience methods are so commonly used that they tend to be declared inline in multiple project locations.
  template <std::integral I, bool byteswap = false> std::pair<Result, I> read(Address address, Operation op) const;
  template <std::integral I, bool byteswap = false> Result write(Address address, I src, Operation op);
};

// If you act like a bus you need to implement this. It allows decoding of packets into their initiator's address space.
struct Translator {
  static constexpr Device::Type TypeMask = Device::Type::MemoryTranslator;
  virtual ~Translator() = default;
  virtual std::tuple<bool, Device::ID, Address> forward(Address address) const = 0;
  virtual std::optional<Address> backward(Device::ID child, Address address) const = 0;
  // virtual void setPathManager(QSharedPointer<api2::Paths> paths) = 0;
  // virtual QSharedPointer<const api2::Paths> pathManager() const = 0;
};

struct Initiator {
  static constexpr Device::Type TypeMask = Device::Type::MemoryInitiator;
  virtual ~Initiator() = default;
  // Sets the memory backing for a particular port (i.e., set the I and D caches
  // separately) If port is nullptr, then all ports will use the target,
  virtual void bind_port(Target *target, std::string_view port_name = {}) = 0;
  virtual const std::span<const std::string> list_ports() const = 0;
  virtual Target *get_port(std::string_view port_name = {}) const = 0;
};

/*
 * Inline implementations
 */
template <std::integral I, bool byteswap>
std::pair<Target::Result, I> Target::read(Address address, Operation op) const {
  I dest;
  auto r = read(address, bits::span<u8>(reinterpret_cast<u8 *>(&dest), sizeof(I)), op);
  if constexpr (byteswap) dest = bits::byteswap(dest);
  return {r, dest};
}
template <std::integral I, bool byteswap> Target::Result Target::write(Address address, I src, Operation op) {
  if constexpr (byteswap) src = bits::byteswap(src);
  return write(address, bits::span<const u8>(reinterpret_cast<const u8 *>(&src), sizeof(I)), op);
}
