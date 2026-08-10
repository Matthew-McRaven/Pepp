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
#include "core/sim/api/device.hpp"

namespace trace {
class Recorder;
}

// A device that can contribute to a trace.
// At System initialization time, the System walks the device tree and binds every traceable to a trace::Recorder.
// A Recorder is a helper class which emits changes as meaningful, invertible tvm programs.
// We do not operate on TB directly, otherwise all Traceables would need to handle enable tracking, which would be
// code duplicated across all devices.
class Traceable {
public:
  static constexpr Device::Type TypeMask = Device::Type::Traceable;
  virtual ~Traceable() = default;
  // Taken by const reference so this header needs only a forward declaration; implementors copy it by value.
  virtual void set_recorder(const trace::Recorder &recorder) = 0;
  // Can this device ever be traced? A debugger or TraceBuffer would answer false would answer false.
  virtual bool can_generate_traces() const = 0;
  // Is this device currently being traced in our TB?
  virtual bool traced() const = 0;
  // Toggle the traced state of this device in our TB. If the device cannot generate traces, this must be a no-op.
  virtual void trace(bool enabled) = 0;
};
