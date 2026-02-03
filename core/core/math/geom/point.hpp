/*
 *  Copyright (c) 2026. Stanley Warford, Matthew McRaven
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
#include <algorithm>
#include <utility>

namespace pepp::core {
template <typename T> struct Point {
  explicit Point() noexcept : _x(T()), _y(T()) {}
  Point(T x, T y) noexcept : _x(x), _y(y) {}
  Point(const Point &) noexcept = default;
  Point &operator=(const Point &) noexcept = default;
  Point(Point &&) noexcept = default;
  Point &operator=(Point &&) noexcept = default;
  friend void swap(Point &lhs, Point &rhs) noexcept {
    using std::swap;
    swap(lhs._x, rhs._x);
    swap(lhs._y, rhs._y);
  }
  ~Point() = default;
  // Also scanline orderm like rectangle, min y then min x.
  auto operator<=>(const Point &other) const {
    if (auto c = _y <=> other._y; c != 0) return c;
    return _x <=> other._x;
  }
  bool operator==(const Point &other) const noexcept = default;
  inline T x() const noexcept { return _x; }
  inline T y() const noexcept { return _y; }

private:
  T _x, _y;
};
} // namespace pepp::core
