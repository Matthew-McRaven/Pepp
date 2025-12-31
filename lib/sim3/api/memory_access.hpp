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
#include <QtCore>
#include "clock.hpp"

namespace sim::api2::memory {
// If select memory operations fail (e.g., lack of MMI, unmapped address in
// bus), specify the behavior of the target.
enum class FailPolicy {
  YieldDefaultValue, // The target picks some arbitrary default value, and
                     // returns it successfully.
  RaiseError         // The target returns an appropriate error message.
};

class Error : public std::runtime_error {
public:
  enum class Type {
    Unmapped,  // Attempted to read a physical address with no device present.
    OOBAccess, // Attempted out-of-bound access on a storage device.
    NeedsMMI,  // Attempted to read MMI that had no buffered input.
    WriteToRO, // Attempt to write to read-only memory.
  };
  template <typename Address> static const std::string format(Address address) {
    auto addrString = QString::number(address, 16);
    return QString("Memory access error at 0x%1").arg(addrString, sizeof(Address) * 2, '0').toStdString();
  }
  template <typename Address>
  Error(Type type, Address address)
      : std::runtime_error(format(address)), _type(type), _address(address), _width(sizeof(Address)){};
  quint64 address() const { return _address; }
  quint8 byte_count() const { return _width; }
  Type type() const { return _type; }

private:
  Type _type;
  quint64 _address, _width;
};

struct Result {
  // Number of simulation ticks required to complete the memory op.
  // 0 indicates the operation completed immediately.
  tick::Type delay;
  // Did the memory access trigger a breakpoint?
  bool pause;
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
  enum class Type : quint8 {
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
} // namespace sim::api2::memory
