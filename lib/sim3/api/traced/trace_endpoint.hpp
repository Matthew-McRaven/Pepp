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
#include "./trace_iterator.hpp"

namespace sim::api2::trace {
class Buffer;
class Source {
public:
  virtual ~Source() = default;
  virtual void setBuffer(Buffer *tb) = 0;
  virtual const Buffer *buffer() const = 0;
  virtual void trace(bool enabled) = 0;
};

class Sink {
public:
  virtual ~Sink() = default;
  // Return true if the packet was processed by this sink, otherwise return
  // false.
  virtual bool analyze(PacketIterator iter, Direction direction) = 0;
};

} // namespace sim::api2::trace
