#pragma once
#include <ostream>
#include <set>

namespace sim::trace2 {
template <typename T> struct Interval {
  explicit Interval() : _lower(T()), _upper(T()) {}
  explicit Interval(T point) : _lower(point), _upper(point) {}
  // Enforce lower <= upper to make math easier.
  Interval(T lower, T upper) : _lower(lower <= upper ? lower : upper), _upper(lower <= upper ? upper : lower) {}
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

private:
  // Prevent writes to _lower and _upper to maintain class invariant of lower() <= upper();
  // This is enforced by the constructor(s).
  T _lower, _upper;
};

template <typename T> bool contains(const Interval<T> &outer, const Interval<T> &inner) {
  return outer.lower() <= inner.lower() && inner.upper() <= outer.upper();
}
template <typename T> bool intersects(const Interval<T> &lhs, const Interval<T> &rhs) {
  return lhs.lower() <= rhs.upper() && rhs.lower() <= lhs.upper();
}
template <typename T> Interval<T> intersection(const Interval<T> &lhs, const Interval<T> &rhs) {
  using std::max;
  using std::min;
  return {min(lhs.lower(), rhs.lower()), max(lhs.upper(), rhs.upper())};
}
template <typename T> std::ostream &operator<<(std::ostream &os, const Interval<T> &interval) {
  return os << "[" << interval.lower() << ", " << interval.upper() << "]";
}

// Class to store and merge intervals of numeric types.
// Good words to google: interval tree, interval set.
// BUG: boundary arithmetic can overflow, so require unsigned to avoid UB.
template <std::unsigned_integral T> class IntervalSet {
  std::set<Interval<T>> _intervals;

public:
  void insert(T point) { insert(Interval<T>(point)); }
  void insert(Interval<T> interval) {
    // The key assumption is that intervals are stored in sorted order, implying that a single insert
    // can only merge consectuive indices.

    // First element !< interval
    auto next = _intervals.lower_bound(interval);
    // Set up iterators for merging+erasing items > interval.
    auto eraseStart = next;
    // Initialize to sentinel value. cend indicates no erasure needed.
    auto eraseEnd = _intervals.cend();

    // Can't prev() something already at the start.
    if (next != _intervals.cbegin()) {
      // Prevent operating on empty set.
      if (auto prev = std::prev(next); prev == _intervals.cend()) {
      } else if (contains(*prev, interval))
        return; // Optimization to avoid processing an insert / merge when containment is met.
      else if (intersects(*prev, interval) || prev->upper() + T(1) == interval.lower()) {
        interval = {prev->lower(), interval.upper()};
        // prev->upper <= interval.upper due to lower_bound.
        // Start the merge process from the previous interval, eliminating an extra erase call.
        eraseStart = eraseEnd = prev;
      }
    }

    // Merge with following intervals.
    for (auto it = eraseStart;
         it != _intervals.end() && (intersects(*it, interval) || it->lower() == interval.upper() + T(1));
         // Set end pointer to the last element that will be erased to avoid it being cend().
         eraseEnd = it++) {
      // Must use max, since input interval may entirely contain it's interval.
      // interval->lower <= it->lower due to lower_bound.
      interval = {interval.lower(), std::max(interval.upper(), it->upper())};
    }
    // Prevent erase if no elements are merged.
    if (eraseEnd != _intervals.cend())
      // second pointer must point to the first element not erased, which is not satisfied by for loop.
      _intervals.erase(eraseStart, std::next(eraseEnd));
    _intervals.insert(interval);
  };
  const std::set<Interval<T>> &intervals() const { return _intervals; }
  void clear() { _intervals.clear(); }
};

template <typename T> std::ostream &operator<<(std::ostream &os, const IntervalSet<T> &set) {
  for (const auto &i : set.intervals())
    os << i;
  return os;
}
} // namespace sim::trace2
