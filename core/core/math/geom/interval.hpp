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
#include "core/math/integers/fixed_point_utils.hpp"
#include "core/math/integers/wider.hpp"
namespace pepp::core {

template <typename T> constexpr T quantum() {
  if constexpr (std::is_integral_v<T>) return T{1};
  else return std::bit_cast<T>(cnl::rep_t<T>{1});
}

// Represent a mathematically closed interval [lower, upper], which is inclusive of both endpoints.
// If lower >= upper, the interval is treated as empty, and all empty intervals are equivalent.
// This interval started as a helper for memory address ranges, for which closed intervals make the most sense.
template <typename T> struct Interval {
  // Construct a canonical empty interval
  explicit Interval();
  explicit Interval(T point) : _lower(point), _upper(point) {}
  Interval(T lower, T upper) : _lower(lower), _upper(upper) {}
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
  auto operator<=>(const Interval &other) const noexcept {
    if (bool lhs_empty = !valid(), rhs_empty = !other.valid(); lhs_empty && rhs_empty)
      return std::strong_ordering::equal;
    else if (lhs_empty) return std::strong_ordering::less;
    else if (rhs_empty) return std::strong_ordering::greater;
    else if (auto cmp = _lower <=> other._lower; cmp != 0) return cmp;
    return _upper <=> other._upper;
  }
  bool operator==(const Interval &other) const noexcept {
    if (bool lhs_empty = !valid(), rhs_empty = !other.valid(); lhs_empty && rhs_empty) return true;
    else if (lhs_empty ^ rhs_empty) return false;
    else return _lower == other._lower && _upper == other._upper;
  }
  inline T lower() const { return _lower; }
  inline T upper() const { return _upper; }
  // If T::invalid() exists, returns true if _upper and _lower are valid AND _lower<=_upper
  // Otherwise returns true if _lower <= _upper.
  bool valid() const noexcept;
  // Unconditionally swap _upper and _lower
  void flip() noexcept;
  Interval<T> flipped() const noexcept;
  // Conditionally swap _upper and _lower if _lower < _upper.
  // If T::invalid() exists, this may not make the interval valid if either endpoint is invalid.
  void normalize() noexcept;
  Interval<T> normalized() const noexcept;

public:
  // _lower<=upper is required for valid intervals. All invalid intervals are treated as equivalent, empty intervals.
  T _lower, _upper;
};

template <typename T> Interval<T>::Interval() {
  if constexpr (requires {
                  T() + quantum<T>();
                  T() + 1;
                }) {
    // Not all tpyes (e.g., source locations) can be incremented.
    // For integer intervals, making _lower=_upper+1 is the easiest
    _lower = T() + quantum<T>();
    _upper = T();
  } else if constexpr (requires { T::invalid(); }) {
    // Otherwise use underlying type's "invalid" value.
    _lower = T::invalid();
    _upper = T::invalid();
  } else {
    static_assert(false, "Failed to construct empty interval");
  }
}

template <typename T> bool Interval<T>::valid() const noexcept {
  if constexpr (requires { T().valid(); }) return _lower.valid() && _upper.valid() && _lower <= _upper;
  else return _lower <= _upper;
}

template <typename T> inline void Interval<T>::flip() noexcept { std::swap(_lower, _upper); }

template <typename T> inline Interval<T> Interval<T>::flipped() const noexcept {
  auto ret = *this;
  ret.flip();
  return ret;
}

template <typename T> inline void Interval<T>::normalize() noexcept {
  if (_lower > _upper) flip();
}

template <typename T> Interval<T> Interval<T>::normalized() const noexcept {
  auto ret = *this;
  ret.normalize();
  return ret;
}

// Two variants of size, depending on if the right endpoint is included. For example, for integers, [0, 0] has size 1 if
// the right endpoint is included, and size 0 if it is not. This is the only algorithm which does not always treat the
// interval as closed.
template <typename T, bool exclude_right = false> wider_type_t<T> size(const Interval<T> &interval) {
  static constexpr wider_type_t<T> offset = exclude_right ? 0 : 1;
  if (!interval.valid()) return wider_type_t<T>{0};
  else return static_cast<wider_type_t<T>>(interval.upper()) - static_cast<wider_type_t<T>>(interval.lower()) + offset;
}
template <typename T> wider_type_t<T> size_inclusive(const Interval<T> &interval) { return size<T, false>(interval); }
template <typename T> wider_type_t<T> size_exclusive(const Interval<T> &interval) { return size<T, true>(interval); }

// Check if the first argument completely contains the second argument.
// For the non-interval overload, inner is casted to the closed interval [inner,inner].
template <typename T> bool contains(const Interval<T> &outer, const T &inner) {
  if (!outer.valid()) return false;
  else return outer.lower() <= inner && inner <= outer.upper();
}
template <typename T> bool contains(const Interval<T> &outer, const Interval<T> &inner) {
  if (!outer.valid() || !inner.valid()) return false;
  return outer.lower() <= inner.lower() && inner.upper() <= outer.upper();
}
// Check if two intervals intersect at any point, even if only at the endpoint.
template <typename T> bool intersects(const Interval<T> &lhs, const Interval<T> &rhs) {
  if (!lhs.valid() || !rhs.valid()) return false;
  return lhs.lower() <= rhs.upper() && rhs.lower() <= lhs.upper();
}
template <typename T> Interval<T> intersection(const Interval<T> &lhs, const Interval<T> &rhs) {
  using std::max, std::min;
  if (!lhs.valid() || !rhs.valid()) return Interval<T>();
  else if (!intersects(lhs, rhs)) return Interval<T>();
  else return {max(lhs.lower(), rhs.lower()), min(lhs.upper(), rhs.upper())};
}
// Return smallest interval which fully contains both lhs and rhs.
template <typename T> Interval<T> hull(const Interval<T> &lhs, const Interval<T> &rhs) {
  using std::max, std::min;
  if (bool lhs_empty = !lhs.valid(), rhs_empty = !rhs.valid(); lhs_empty && rhs_empty) return Interval<T>();
  else if (lhs_empty) return rhs;
  else if (rhs_empty) return lhs;
  else return {min(lhs.lower(), rhs.lower()), max(lhs.upper(), rhs.upper())};
}

template <typename T> std::ostream &operator<<(std::ostream &os, const Interval<T> &interval) {
  return os << "[" << interval.lower() << ", " << interval.upper() << "]";
}
// Overload to pretty print intervals of integers.
template <std::integral I> std::ostream &operator<<(std::ostream &os, const Interval<I> &interval) {
  // We have many i8 / u8 intervals that like to render their contents as characters
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
} // namespace pepp::core
