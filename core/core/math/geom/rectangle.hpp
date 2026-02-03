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
#include "core/math/geom/interval.hpp"
#include "core/math/geom/point.hpp"

namespace pepp::core {

template <typename T> struct Rectangle {
  Rectangle() : _x(Interval<T>()), _y(Interval<T>()) {}
  explicit Rectangle(Point<T> pt) noexcept : _x(pt.x(), pt.x()), _y(pt.y(), pt.y()) {}
  Rectangle(Interval<T> x, Interval<T> y) : _x(std::move(x)), _y(std::move(y)) {}
  Rectangle(Point<T> top_left, Point<T> bottom_right) noexcept {
    auto min_x = std::min(top_left.x(), bottom_right.x());
    auto max_x = std::max(top_left.x(), bottom_right.x());
    _x = Interval<T>(min_x, max_x);
    auto min_y = std::min(top_left.y(), bottom_right.y());
    auto max_y = std::max(top_left.y(), bottom_right.y());
    _y = Interval<T>(min_y, max_y);
  }

  Rectangle(const Rectangle &) noexcept = default;
  Rectangle &operator=(const Rectangle &) noexcept = default;
  Rectangle(Rectangle &&) noexcept = default;
  Rectangle &operator=(Rectangle &&) noexcept = default;
  friend void swap(Rectangle &lhs, Rectangle &rhs) noexcept {
    using std::swap;
    swap(lhs._x, rhs._x);
    swap(lhs._y, rhs._y);
  }
  ~Rectangle() = default;
  // Total order based on min(y), then min(x), then max(y), then max(x).
  // This is trying to sort rectangles in an order than is appropriate for scanline rendering.
  // See: https://en.wikipedia.org/wiki/Scanline_rendering
  auto operator<=>(const Rectangle &other) const noexcept {
    if (auto c = _y.lower() <=> other._y.lower(); c != 0) return c;
    if (auto c = _x.lower() <=> other._x.lower(); c != 0) return c;
    if (auto c = _y.upper() <=> other._y.upper(); c != 0) return c;
    return _x.upper() <=> other._x.upper();
  }
  bool operator==(const Rectangle &other) const noexcept = default;

  T height() const noexcept { return _y.upper() - _y.lower(); }
  T width() const noexcept { return _x.upper() - _x.lower(); }
  // These are screen-ish coordinates, with y increasing downward.
  Point<T> top_left() const noexcept { return {_x.lower(), _y.lower()}; };
  Point<T> bottom_right() const noexcept { return {_x.upper(), _y.upper()}; };
  const Interval<T> &x() const noexcept { return _x; }
  const Interval<T> &y() const noexcept { return _y; }

private:
  Interval<T> _x, _y;
};

template <typename T> std::size_t area(const Rectangle<T> &rect) { return rect.height() * rect.width(); }

template <typename T> bool contains(const Rectangle<T> &rect, const Point<T> &inner) {
  return contains(rect.x(), inner.x()) && contains(rect.y(), inner.y());
}
template <typename T> bool contains(const Rectangle<T> &outer, const Rectangle<T> &inner) {
  return contains(outer.x(), inner.x()) && contains(outer.y(), inner.y());
}
template <typename T> bool intersects(const Rectangle<T> &lhs, const Rectangle<T> &rhs) {
  return intersects(lhs.x(), rhs.x()) && intersects(lhs.y(), rhs.y());
}
template <typename T> Rectangle<T> intersection(const Rectangle<T> &lhs, const Rectangle<T> &rhs) {
  return {intersection(lhs.x(), rhs.x()), intersection(lhs.y(), rhs.y())};
}
// Smallest rectangle which contains lhs and rhs.
template <typename T> Rectangle<T> hull(const Rectangle<T> &lhs, const Rectangle<T> &rhs) {
  return {hull(lhs.x(), rhs.x()), hull(lhs.y(), rhs.y())};
}

template <typename T> std::ostream &operator<<(std::ostream &os, const Rectangle<T> &interval) {
  return os << "Rectangle(" << interval.x() << ", " << interval.y() << ")";
}
} // namespace pepp::core
