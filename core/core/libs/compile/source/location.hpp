/*
 * /Copyright (c) 2026. Stanley Warford, Matthew McRaven
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
#include <type_traits>
#include "core/integers.h"
#include "core/libs/math/interval.hpp"

namespace pepp::tc::support {
// Represent a point (i.e., a single character) within a text file.
struct Location {
  Location() = default;
  Location(u16 r, u16 c);
  // A location is considered invalid if either its row or column is INVALID.
  bool valid() const;
  auto operator<=>(const Location &other) const = default;
  bool operator==(const Location &other) const = default;

  static constexpr u16 INVALID = -1;
  static constexpr u16 MAX = -2;
  u16 row = INVALID;
  u16 column = INVALID;
};

// Represents a range of characters in a text file.
// It does not know which file it is in. You must track this yourself
struct LocationInterval : public pepp::core::Interval<Location> {
  LocationInterval() = default;
  LocationInterval(Location point);
  LocationInterval(Location lower, Location upper);
  // The interval is invalid if either its lower or upper bound is invalid.
  bool valid() const;
};

// These types will be constructed a lot. I want them to work like normal value types (int).
// As I add features, I'm worried I will break copy/move.
static_assert(std::is_copy_constructible_v<Location>, "Location must be copy constructible");
static_assert(std::is_copy_assignable_v<Location>, "Location must be copy assignable");
static_assert(std::is_move_constructible_v<Location>, "Location must be move constructible");
static_assert(std::is_move_assignable_v<Location>, "Location must be move assignable");
static_assert(std::is_copy_constructible_v<LocationInterval>, "LocationInterval must be copy constructible");
static_assert(std::is_copy_assignable_v<LocationInterval>, "LocationInterval must be copy assignable");
static_assert(std::is_move_constructible_v<LocationInterval>, "LocationInterval must be move constructible");
static_assert(std::is_move_assignable_v<LocationInterval>, "LocationInterval must be move assignable");

} // namespace pepp::tc::support
