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
#include "core/integers.h"
#include "core/math/geom/interval.hpp"
#include "core/math/geom/point.hpp"

namespace pepp::core {

template <typename T> struct Rectangle {
  Rectangle() : _x(Interval<T>()), _y(Interval<T>()) {}
  explicit Rectangle(Point<T> pt) : _x(pt.x()), _y(pt.y()) {}
  // Caller must provide a non-zero size
  Rectangle(Point<T> pt, Size<T> size)
      : _x(pt.x(), pt.x() + size.width() - 1), _y(pt.y(), pt.y() + size.height() - 1) {}
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

  T height() const noexcept { return _y.upper() - _y.lower() + 1; }
  T width() const noexcept { return _x.upper() - _x.lower() + 1; }
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

template <typename T> std::ostream &operator<<(std::ostream &os, const Rectangle<T> &rect) {
  return os << "Rectangle(" << rect.x() << ", " << rect.y() << ")";
}

// Decomposes a rectangle into an iterable of non-overlapping rectangles that cover the same area.
// Resulting rectangles will be aligned to an 8x8 grid.
template <typename T> class RectangleDecomposer {
public:
  RectangleDecomposer(Rectangle<T> rect) noexcept : _rect(rect) {}
  RectangleDecomposer(Point<T> pt, Size<T> size = {0, 0}) noexcept : _rect(pt, size) {}
  // End iterator is represented as any iterator >8y below the rectangle
  // Multiple ~T(7) to mask out the low-order 3-bits, which corresponds to our 8x8 grid.
  struct Iterator {
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::pair<Point<T>, Rectangle<u8>>;
    Iterator() noexcept : _src(), _pos(0, 9) {}
    Iterator(Rectangle<T> rect) noexcept : _src(rect) {
      const auto init = _src.top_left();
      // Mask off low bits to get 8x8 grid aligned starting coordinate.
      const auto x = init.x() & ~T(7), y = init.y() & ~T(7);
      _pos = Point<T>(x, y);
    }
    // Computes the end iterator for a given rectangle.
    static Iterator end_for(Rectangle<T> rect) {
      Iterator it(rect);
      it._pos = Point<T>(rect.x().lower() & ~T(7), (rect.y().upper() & ~T(7)) + 8);
      return it;
    }

    value_type operator*() const noexcept {
      const Rectangle<T> rect_mask(_pos, Size<T>{8, 8});
      const auto isec = intersection(_src, rect_mask);
      // "reproject" the intersection to be relative to the (0,0) of the 8x8 mask
      const u8 clipped_x = static_cast<u8>(isec.x().lower() - rect_mask.x().lower());
      const u8 clipped_y = static_cast<u8>(isec.y().lower() - rect_mask.y().lower());
      const u8 clipped_w = static_cast<u8>(isec.width());
      const u8 clipped_h = static_cast<u8>(isec.height());
      const Rectangle<u8> isec_u8(Point<u8>(clipped_x, clipped_y), Size<u8>(clipped_w, clipped_h));
      return std::make_pair(_pos, isec_u8);
    }
    Iterator &operator++() noexcept {
      _pos = Point<T>(_pos.x() + 8, _pos.y());
      if (_pos.x() > _src.x().upper()) _pos = Point<T>(_src.x().lower() & ~T(7), _pos.y() + 8);
      return *this;
    }
    Iterator operator++(int) {
      auto prev = *this;
      ++*this;
      return prev;
    }
    bool operator==(const Iterator &other) const noexcept = default;
    auto operator<=>(const Iterator &other) const noexcept = default;

  private:
    Rectangle<T> _src;
    Point<T> _pos;
  };
  Iterator begin() const noexcept { return Iterator(_rect); }
  Iterator end() const noexcept { return Iterator::end_for(_rect); }

private:
  Rectangle<T> _rect;
};
} // namespace pepp::core
