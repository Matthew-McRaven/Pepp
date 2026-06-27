/*
 *  Copyright (c) 2024-2026. Stanley Warford, Matthew McRaven
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once
#include <array>
#include <system_error>
#include <zpp_bits.h>
#include "core/integers.h"
#include "core/math/bitmanip/copy.hpp"
#include "core/math/bitmanip/span.hpp"
#include "core/sim/api/device.hpp"

class Buffer {
public:
  virtual ~Buffer() = default;

  // Create a new frame header and write it to the buffer
  virtual bool begin_group(Device::ID which_device) = 0;
  virtual bool write_packet(Device::ID group, bits::span<const u8> packet) = 0;

  // Deriving classes MUST also call this implementation of clear() if overriding it.
  virtual void clear() = 0;

  // virtual FrameIterator cbegin() const = 0;
  // virtual FrameIterator cend() const = 0;
  // virtual FrameIterator crbegin() const = 0;
  // virtual FrameIterator crend() const = 0;

  virtual bool trace(Device::ID deviceID, bool enabled = true) = 0;
  virtual bool traced(Device::ID deviceID) const = 0;
};

namespace packets {
enum class Types : u8 {
  // no payload nor address
  Invalid = 0,
  Clear = 1,
  // no payload but with address
  PureRead = 2,
  // with payload and address
  ImpureRead = 3,
  Write = 4,
  Increment = 5,
};

struct Clear {
  static constexpr Types type = Types::Clear;
  Device::ID device;
};

struct PureRead {
  static constexpr Types type = Types::PureRead;
  Device::ID device;
  u32 address, payload_len;
};

struct ImpureRead {
  static constexpr Types type = Types::ImpureRead;
  Device::ID device;
  u32 address, payload_len;
};

struct Write {
  static constexpr Types type = Types::Write;
  Device::ID device;
  u32 address, payload_len;
};

struct Increment {
  static constexpr Types type = Types::Increment;
  Device::ID device;
  u32 address, payload_len;
};

} // namespace packets
