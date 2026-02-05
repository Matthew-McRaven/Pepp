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
#include <assert.h>
#include <ostream>
#include "core/integers.h"

namespace pepp::core {
template <typename T> struct Interval {
  explicit Interval() : _lower(T()), _upper(T()) {}
  explicit Interval(T point) : _lower(point), _upper(point) {}
  // Enforce lower <= upper to make math easier.
  Interval(T lower, T upper) : _lower(lower), _upper(upper) { assert(lower <= upper); }
  Interval(const Interval &) = default;
  Interval &operator=(const Interval &) = default;
  Interval(Interval &&) = default;
  Interval &operator=(Interval &&) = default;
  friend void swap(Interval &lhs, Interval &rhs) {
    using std::swap;
    swap(lhs._lower, rhs._lower);
    swap(lhs._upper, rhs._upper);
  }
  ~Interval() = default;
  auto operator<=>(const Interval &other) const = default;
  inline T lower() const { return _lower; }
  inline T upper() const { return _upper; }

public:
  // Prevent writes to _lower and _upper to maintain class invariant of lower() <= upper();
  // This is enforced by the constructor(s).
  T _lower, _upper;
};

template <typename T, bool exclude_right = false> std::size_t size(const Interval<T> &interval) {
  static constexpr std::size_t offset = exclude_right ? 0 : 1;
  return static_cast<std::size_t>(interval.upper()) - static_cast<std::size_t>(interval.lower()) + offset;
}
template <typename T> std::size_t size_inclusive(const Interval<T> &interval) { return size<T, false>(interval); }
template <typename T> std::size_t size_exclusive(const Interval<T> &interval) { return size<T, true>(interval); }
template <typename T> bool contains(const Interval<T> &outer, const T &inner) {
  return outer.lower() <= inner && inner <= outer.upper();
}
template <typename T> bool contains(const Interval<T> &outer, const Interval<T> &inner) {
  return outer.lower() <= inner.lower() && inner.upper() <= outer.upper();
}
template <typename T> bool intersects(const Interval<T> &lhs, const Interval<T> &rhs) {
  return lhs.lower() <= rhs.upper() && rhs.lower() <= lhs.upper();
}
template <typename T> Interval<T> intersection(const Interval<T> &lhs, const Interval<T> &rhs) {
  using std::max, std::min;
  assert(intersects(lhs, rhs));
  return {max(lhs.lower(), rhs.lower()), min(lhs.upper(), rhs.upper())};
}
// Return smallest interval which contains both.
template <typename T> Interval<T> hull(const Interval<T> &lhs, const Interval<T> &rhs) {
  using std::max, std::min;
  return {min(lhs.lower(), rhs.lower()), max(lhs.upper(), rhs.upper())};
}
template <typename T> std::ostream &operator<<(std::ostream &os, const Interval<T> &interval) {
  return os << "[" << (i64)interval.lower() << ", " << (i64)interval.upper() << "]";
}

// Helper to translate a value in a src interval into an offset, then translate that offset into a destintation
// interval. It is essentially segmented address translation.
template <typename T> inline T offset_map(T v, const Interval<T> &src, const Interval<T> &dst) {
  assert(src.lower() <= v && v <= src.upper());
  T key_src_offet = v - src.lower();
  T ret = dst.lower() + key_src_offet;
  assert(dst.lower() <= ret && ret <= dst.upper());
  return ret;
}
}
