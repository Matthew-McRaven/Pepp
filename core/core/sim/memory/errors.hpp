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
#include <stdexcept>
#include "core/integers.h"
#include "fmt/format.h"

class Error : public std::runtime_error {
public:
  enum class Type {
    Unmapped,  // Attempted to read a physical address with no device present.
    OOBAccess, // Attempted out-of-bound access on a storage device.
    NeedsMMI,  // Attempted to read MMI that had no buffered input.
    WriteToRO, // Attempt to write to read-only memory.
  };
  template <typename Address> static const std::string format(Address address) {
    return fmt::format("Memory access error at 0x{:08x}", address);
  }
  template <typename Address>
  Error(Type type, Address address)
      : std::runtime_error(format(address)), _type(type), _address(address), _width(sizeof(Address)){};
  u64 address() const { return _address; }
  u8 byte_count() const { return _width; }
  Type type() const { return _type; }

private:
  Type _type;
  u64 _address, _width;
};
