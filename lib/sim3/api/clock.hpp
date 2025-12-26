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

// In API v1, errors are communicated via an Error field.
// In API v2 (this version), errors are communicated via exceptions.
namespace sim::api2::tick {
using Type = quint32;
struct Result {
  // After this tick, should control be returned to execution enviroment?
  bool pause = false;
  // Number of ticks required before this device can be clock'ed again.
  // If you need a 1 cycle delay, your delay would be 1*source->interval();
  // 2 tick delay is 1*source->interval(), etc.
  Type delay;
};

class Source {
public:
  virtual ~Source() = default;
  // Number of ticks between clock cycles.
  virtual Type interval() const = 0;
};

class Recipient {
public:
  virtual ~Recipient() = default;
  virtual const Source *getSource() = 0;
  virtual void setSource(Source *) = 0;
  virtual Result clock(tick::Type currentTick) = 0;
};
} // namespace sim::api2::tick
