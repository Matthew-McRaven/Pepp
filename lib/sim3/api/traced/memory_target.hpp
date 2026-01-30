/*
 * Copyright (c) 2024 J. Stanley Warford, Matthew McRaven
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
#include "../device.hpp"
#include "../memory_access.hpp"
#include "../memory_address.hpp"
#include "core/libs/bitmanip/span.hpp"
#include "memory_path.hpp"
namespace sim::api2::memory {
template <typename Address> struct Target {
  virtual ~Target() = default;
  // Needed by Translators to perform standalone address translation.
  virtual device::ID deviceID() const = 0;
  virtual device::Descriptor device() const = 0;

  virtual AddressSpan<Address> span() const = 0;
  virtual Result read(Address address, bits::span<quint8> dest, Operation op) const = 0;
  virtual Result write(Address address, bits::span<const quint8> src, Operation op) = 0;
  virtual void clear(quint8 fill) = 0;

  // If dest is larger than maxOffset-minOffset+1, copy bytes from this target
  // to the span.
  virtual void dump(bits::span<quint8> dest) const = 0;
};

// If you act like a bus you need to implement this. It allows decoding of packets into their initiator's address space.
template <typename Address> struct Translator {
  virtual ~Translator() = default;
  virtual std::tuple<bool, device::ID, Address> forward(Address address) const = 0;
  virtual std::optional<Address> backward(device::ID child, Address address) const = 0;
  virtual void setPathManager(QSharedPointer<api2::Paths> paths) = 0;
  virtual QSharedPointer<const api2::Paths> pathManager() const = 0;
};

template <typename Address> struct Initiator {
  virtual ~Initiator() = default;
  // Sets the memory backing for a particular port (i.e., set the I and D caches
  // separately) If port is nullptr, then all ports will use the target,
  virtual void setTarget(Target<Address> *target, void *port = nullptr) = 0;
};
} // namespace sim::api2::memory
