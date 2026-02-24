/*
 * /Copyright (c) 2024-2025. Stanley Warford, Matthew McRaven
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
#include "clock.hpp"
#include "device.hpp"
#include "traced/memory_path.hpp"
namespace sim::api2 {
struct Scheduler {
  enum class Mode {
    Increment, // Execute only the next tick, even if no clocked device is
    // ticked.
    Jump, // Execute up to and including the next tick with a clocked device.
  };
  virtual ~Scheduler() = default;
  virtual tick::Recipient *next(tick::Type current, Mode mode) = 0;
  virtual void schedule(tick::Recipient *listener, tick::Type startingOn) = 0;
  virtual void reschedule(device::ID device, tick::Type startingOn) = 0;
};
namespace trace {
class Buffer;
}
template <typename Address> struct System {
  virtual ~System() = default;
  // Returns (current tick, result of ticking that clocked device).
  // Implementors are responsible for creating a frame packet in the TB at the
  // start of the function, and updating the frame header packer at the end of
  // the function.
  // TODO: clock all devices who are scheduled for the next tick.
  virtual std::pair<tick::Type, tick::Result> tick(Scheduler::Mode mode) = 0;
  virtual tick::Type currentTick() const = 0;
  virtual device::ID nextID() = 0;
  virtual device::IDGenerator nextIDGenerator() = 0;
  virtual void addDevice(device::Descriptor) = 0;
  virtual device::Descriptor *descriptor(device::ID) = 0;

  virtual void setBuffer(trace::Buffer *buffer) = 0;
  virtual QSharedPointer<const api2::Paths> pathManager() const = 0;
};
} // namespace sim::api2
