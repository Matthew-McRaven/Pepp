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
// I have multiple classes which act as a vector with two components.
// I can add additional functionality (magnitude, element-wise reductions) and other classes gain this functionality for
// free.
template <typename T> struct Vec2 {
  explicit Vec2() noexcept : elements{T(), T()} {}
  Vec2(T x, T y) noexcept : elements{x, y} {}
  Vec2(const Vec2 &) noexcept = default;
  Vec2 &operator=(const Vec2 &) noexcept = default;
  Vec2(Vec2 &&) noexcept = default;
  Vec2 &operator=(Vec2 &&) noexcept = default;
  friend void swap(Vec2 &lhs, Vec2 &rhs) noexcept {
    using std::swap;
    swap(lhs.elements, rhs.elements);
  }
  ~Vec2() = default;
  T elements[2];
  bool operator==(const Vec2 &other) const noexcept = default;
};

// A single point in 2D space
template <typename T> class Point : private Vec2<T> {
public:
  explicit Point() noexcept : Vec2<T>() {}
  Point(T x, T y) noexcept : Vec2<T>(x, y) {}
  Point(const Point &) noexcept = default;
  Point &operator=(const Point &) noexcept = default;
  Point(Point &&) noexcept = default;
  Point &operator=(Point &&) noexcept = default;
  friend void swap(Point &lhs, Point &rhs) noexcept {
    using std::swap;
    swap(static_cast<Vec2<T> &>(lhs), static_cast<Vec2<T> &>(rhs));
  }
  ~Point() = default;

  // Also scanline order, like rectangle. min y then min x.
  auto operator<=>(const Point &other) const {
    if (auto c = y() <=> other.y(); c != 0) return c;
    return x() <=> other.x();
  }
  bool operator==(const Point &other) const noexcept = default;

  inline T x() const noexcept { return this->elements[0]; }
  inline T y() const noexcept { return this->elements[1]; }
};

// A size of an entity in 2d space, often combined with a point to form a rectangle.
template <typename T> class Size : private Vec2<T> {
public:
  explicit Size() noexcept : Vec2<T>() {}
  Size(T width, T height) noexcept : Vec2<T>(width, height) {}
  Size(const Size &) noexcept = default;
  Size &operator=(const Size &) noexcept = default;
  Size(Size &&) noexcept = default;
  Size &operator=(Size &&) noexcept = default;
  friend void swap(Size &lhs, Size &rhs) noexcept {
    using std::swap;
    swap(static_cast<Vec2<T> &>(lhs), static_cast<Vec2<T> &>(rhs));
  }
  ~Size() = default;

  // Also scanline order, like rectangle. min y then min x.
  auto operator<=>(const Size &other) const {
    if (auto c = height() <=> other.height(); c != 0) return c;
    return width() <=> other.width();
  }
  bool operator==(const Size &other) const noexcept = default;

  inline T width() const noexcept { return this->elements[0]; }
  inline T height() const noexcept { return this->elements[1]; }
};
} // namespace pepp::core
