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

namespace sim::api2::device {
using ID = quint16; // Only use 9 bits (max of 512)!
struct Descriptor {
  device::ID id; // Must uniquely identify a device within a system.
  void *compatible;
  QString baseName, fullName;
};
using IDGenerator = std::function<ID()>;
} // namespace sim::api2::device
