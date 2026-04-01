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

// Represent a rectangular region of 2D space.
// While you can construct it from a pair of points, a point+size, or a single point, the rectangle itself is always
// represented as a pair of intervals, one for x and one for y. This was an implementation decision -- an interval is
// the equivalent in 1D to a rectangle. Reusing intervals as the internal representation means we can reuse our existing
// algorithm.
template <typename T> struct Rectangle {
  // Creates an invalid rectangle, which needs to be guarded against in operations.
  explicit Rectangle() : _x(Interval<T>()), _y(Interval<T>()) {}
  explicit Rectangle(Point<T> pt) : _x(pt.x()), _y(pt.y()) {}
  // Caller must provide a non-zero size
  Rectangle(Point<T> pt, Size<T> size)
      : _x(pt.x(), pt.x() + size.width() - quantum<T>()), _y(pt.y(), pt.y() + size.height() - quantum<T>()) {}
  Rectangle(Interval<T> x, Interval<T> y) : _x(std::move(x)), _y(std::move(y)) {}
  Rectangle(Point<T> top_left, Point<T> bottom_right) noexcept;
  static constexpr Rectangle from_point_point(T x1, T y1, T x2, T y2) noexcept;
  static constexpr Rectangle from_point_size(T x, T y, T width, T height) noexcept;

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
  auto operator<=>(const Rectangle &other) const noexcept;
  bool operator==(const Rectangle &other) const noexcept = default;

  T height() const noexcept { return size_inclusive(_y); }
  T width() const noexcept { return size_inclusive(_x); }
  Size<T> size() const noexcept { return Size<T>(width(), height()); }
  // These are screen-ish coordinates, with y increasing downward.
  Point<T> top_left() const noexcept { return {_x.lower(), _y.lower()}; };
  Point<T> bottom_right() const noexcept { return {_x.upper(), _y.upper()}; };
  Point<T> midpoint_and_b() const noexcept { return Point<T>(_x.midpoint_approximate(), _y.midpoint_approximate()); }
  const Interval<T> &x() const noexcept { return _x; }
  const Interval<T> &y() const noexcept { return _y; }
  //  x and y do not return actual points. Discuss with Matthew if we can rename x and y above
  //  It will have big impact on code.
  const T left() const noexcept { return _x.lower(); }
  const T right() const noexcept { return _x.upper(); }
  const T top() const noexcept { return _y.lower(); }
  const T bottom() const noexcept { return _y.upper(); }
  bool valid() const noexcept { return _x.valid() && _y.valid(); }
  // If either x or y is reversed (lower > upper), then the rectangle is invalid, which might happen with geometric
  // manipulation of this class. Normalize swaps any reversed intervals.
  Rectangle<T> normalized() const noexcept;
  void normalize() noexcept;
  // Helpers to adjust the points of the rectangle by a different delta for each point.
  void adjust(T delta_left, T delta_top, T delta_right, T delta_bottom) noexcept;
  Rectangle<T> adjusted(T delta_left, T delta_top, T delta_right, T delta_bottom) const noexcept;
  // Adjust the rectangle using a single delta for x and a single delta for y.
  // It preserves the size of the rectangle and preserves the valid state.
  void translate(T delta_x, T delta_y) noexcept;
  void translate(Point<T> delta) noexcept;
  Rectangle<T> translated(T delta_x, T delta_y) const noexcept;
  Rectangle<T> translated(Point<T> delta) const noexcept;
  // Keeping the same top-left corner, swap the height and width of the rectangle.
  void transpose() noexcept;
  Rectangle<T> transposed() const noexcept;

private:
  Interval<T> _x, _y;
};

template <typename T> inline auto Rectangle<T>::operator<=>(const Rectangle &other) const noexcept {
  if (bool lhs_empty = !valid(), rhs_empty = !other.valid(); lhs_empty && rhs_empty) return std::strong_ordering::equal;
  else if (lhs_empty) return std::strong_ordering::less;
  else if (rhs_empty) return std::strong_ordering::greater;
  else if (auto c = _y.lower() <=> other._y.lower(); c != 0) return c;
  else if (auto c = _x.lower() <=> other._x.lower(); c != 0) return c;
  else if (auto c = _y.upper() <=> other._y.upper(); c != 0) return c;
  return _x.upper() <=> other._x.upper();
}

template <typename T> void Rectangle<T>::normalize() noexcept { _x.normalize(), _y.normalize(); }

template <typename T> Rectangle<T> Rectangle<T>::normalized() const noexcept {
  auto ret = *this;
  ret.normalize();
  return ret;
}

template <typename T> inline Rectangle<T>::Rectangle(Point<T> top_left, Point<T> bottom_right) noexcept {
  _x = Interval<T>(top_left.x(), bottom_right.x());
  _y = Interval<T>(top_left.y(), bottom_right.y());
}

template <typename T> constexpr Rectangle<T> Rectangle<T>::from_point_point(T x1, T y1, T x2, T y2) noexcept {
  return Rectangle(Point<T>(x1, y1), Point<T>(x2, y2));
}

template <typename T> constexpr Rectangle<T> Rectangle<T>::from_point_size(T x, T y, T width, T height) noexcept {
  return Rectangle(Point<T>(x, y), Size<T>(width, height));
}

// Use auto here to auto
template <typename T> auto area(const Rectangle<T> &rect) -> std::invoke_result_t<std::multiplies<>, T, T> {
  if (!rect.valid()) return 0;
  else return rect.height() * rect.width();
}

template <typename T> void Rectangle<T>::adjust(T delta_left, T delta_top, T delta_right, T delta_bottom) noexcept {
  _x = Interval<T>(_x.lower() + delta_left, _x.upper() + delta_right);
  _y = Interval<T>(_y.lower() + delta_top, _y.upper() + delta_bottom);
}

template <typename T>
Rectangle<T> Rectangle<T>::adjusted(T delta_left, T delta_top, T delta_right, T delta_bottom) const noexcept {
  auto ret = *this;
  ret.adjust(delta_left, delta_top, delta_right, delta_bottom);
  return ret;
}

template <typename T> void Rectangle<T>::translate(T delta_x, T delta_y) noexcept {
  adjust(delta_x, delta_y, delta_x, delta_y);
}

template <typename T> void Rectangle<T>::translate(Point<T> delta) noexcept { translate(delta.x(), delta.y()); }

template <typename T> Rectangle<T> Rectangle<T>::translated(T delta_x, T delta_y) const noexcept {
  auto ret = *this;
  ret.translate(delta_x, delta_y);
  return ret;
}

template <typename T> Rectangle<T> Rectangle<T>::translated(Point<T> delta) const noexcept {
  return translated(delta.x(), delta.y());
}

template <typename T> void Rectangle<T>::transpose() noexcept {
  const T _width = width(), _height = height();
  *this = Rectangle<T>(Point<T>(_x.lower(), _y.lower()), Size<T>(_height, _width));
}

template <typename T> Rectangle<T> Rectangle<T>::transposed() const noexcept {
  auto ret = *this;
  ret.transpose();
  return ret;
}

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

// Smallest rectangle which containing the entirety ofboth lhs and rhs.
template <typename T> Rectangle<T> hull(const Rectangle<T> &lhs, const Rectangle<T> &rhs) {
  return {hull(lhs.x(), rhs.x()), hull(lhs.y(), rhs.y())};
}

template <typename T> std::ostream &operator<<(std::ostream &os, const Rectangle<T> &rect) {
  return os << "Rectangle(" << rect.x() << ", " << rect.y() << ")";
}

// Convert a scaled_integer rectangle to its underlying representation.
// If the rectangle is already an integer, this is a no-op.
template <typename T> Rectangle<cnl::rep_t<T>> to_underlying_rect(const Rectangle<T> &rect) {
  if constexpr (std::is_same_v<T, cnl::rep_t<T>>) {
    return rect;
  } else {
    auto to_rep = [](T v) -> cnl::rep_t<T> { return cnl::_impl::to_rep(v); };
    return Rectangle<cnl::rep_t<T>>(Point<cnl::rep_t<T>>(to_rep(rect.x().lower()), to_rep(rect.y().lower())),
                                    Point<cnl::rep_t<T>>(to_rep(rect.x().upper()), to_rep(rect.y().upper())));
  }
}

// The SparseOccupancyGrid wants to clip a larger rectangle to a variety of aligned 8x8 grids.
// It was easier to implement this "decomposition" of a large rectangle into many smaller ones as a reusable component
// than in-place. This class breaks the input rectangle into non-overlapping rectangles that cover the same area, but
// are aligned to an 8x8 grid.
//
// While you *can* use this with a fractional integer, it will probably not be helpful.
// The goal of this class is to break a large rectangle into 8x8 bit chunks which map to the bits of an occupancy grid.
// When using a scaled integer, an 8x8 rectangle spans arbitrarily large number of 8x8 bit chunks.
template <typename T> class RectangleDecomposer {
public:
  RectangleDecomposer(Rectangle<T> rect) noexcept : _rect(rect) {}
  RectangleDecomposer(Point<T> pt, Size<T> size = {0, 0}) noexcept : _rect(pt, size) {}

  // oeprator* returns a point and a rectangle that is at most 8x8 with values clipped to the range [0,7]
  // The point is the top-left corner of the 8x8 grid that the rectangle is aligned to, and the rectangle is relative to
  // the 8x8 grid. If you wanted to get the rectangle in absolute coordinates, you would add the point to the rectangle.
  struct Iterator {
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::pair<Point<T>, Rectangle<u8>>;
    Iterator() noexcept : _src(), _pos(0, 9) {}
    Iterator(Rectangle<T> rect) noexcept;
    // Computes the end iterator for a given rectangle.
    // End iterator is represented as any iterator >8y below the rectangle

    static Iterator end_for(Rectangle<T> rect);

    value_type operator*() const noexcept;
    Iterator &operator++() noexcept;
    Iterator operator++(int);
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

template <typename T> typename RectangleDecomposer<T>::Iterator RectangleDecomposer<T>::Iterator::operator++(int) {
  auto prev = *this;
  ++*this;
  return prev;
}

template <typename T>
typename RectangleDecomposer<T>::Iterator &RectangleDecomposer<T>::Iterator::operator++() noexcept {
  _pos = Point<T>(_pos.x() + 8, _pos.y());
  if (_pos.x() > _src.x().upper()) _pos = Point<T>(_src.x().lower() & ~T(7), _pos.y() + 8);
  return *this;
}

template <typename T>
typename RectangleDecomposer<T>::Iterator::value_type RectangleDecomposer<T>::Iterator::operator*() const noexcept {
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

template <typename T>
typename RectangleDecomposer<T>::Iterator RectangleDecomposer<T>::Iterator::end_for(Rectangle<T> rect) {
  Iterator it(rect);
  // Multiple ~T(7) to mask out the low-order 3-bits, which corresponds to our 8x8 grid.
  it._pos = Point<T>(rect.x().lower() & ~T(7), (rect.y().upper() & ~T(7)) + 8);
  return it;
}

template <typename T> inline RectangleDecomposer<T>::Iterator::Iterator(Rectangle<T> rect) noexcept : _src(rect) {
  const auto init = _src.top_left();
  // Mask off low bits to get 8x8 grid aligned starting coordinate.
  const auto x = init.x() & ~T(7), y = init.y() & ~T(7);
  _pos = Point<T>(x, y);
}

} // namespace pepp::core
