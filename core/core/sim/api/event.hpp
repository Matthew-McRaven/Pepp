/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
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

#include <algorithm>
#include <vector>
#include "core/sim/api/device.hpp"

class Target;
/*
 * Synchronous notifications between devices, representing sideband/wiring at the gat level that this mirco-architecture
 * simulator does not model. e.g., a clock gate snooping a memory-mapped register. Not intended for high throughput.
 */
struct Event {
  virtual ~Event() = default;
};

// Device which recieves events from an EventSource.
struct EventSink {
  static constexpr Device::Type TypeMask = Device::Type::EventSink;
  virtual ~EventSink() = default;
  virtual void on_event(Device::ID from, const Event &event) = 0;
};

// Produces events that are broadcast to all subscribed EventSinks.
class EventSource {
public:
  static constexpr Device::Type TypeMask = Device::Type::EventSource;
  virtual ~EventSource() = default;
  void subscribe(EventSink *sink);
  void unsubscribe(EventSink *sink);
  void unsubscribe_all();

protected:
  void raise(Device::ID from, const Event &event);

private:
  std::vector<EventSink *> _sinks;
};
