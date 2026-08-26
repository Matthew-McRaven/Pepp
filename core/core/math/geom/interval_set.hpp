#pragma once

#include <algorithm>
#include <span>
#include <vector>
#include "core/math/geom/interval.hpp"
namespace pepp::core {

// Class to store and merge intervals of numeric types.
// Good words to google: interval tree, interval set.
template <std::unsigned_integral T> class IntervalSet {
  // Sorted by lower endpoint, and no two elements overlap or are adjacent. This implies they are sorted by upper
  // endpoint too. Consecutive elements are always separated by at least one uncovered value, because adjacency would
  // have caused a merge.
  std::vector<Interval<T>> _intervals;

public:
  void insert(T lower, T upper) { insert(Interval<T>(lower, upper)); }
  void insert(T point) { insert(Interval<T>(point)); }
  void insert(Interval<T> interval) {
    if (!interval.valid()) return;
    // first is the least whose upper endpoint is at least interval.lower() - 1.
    // last is the least element whose lower endpoint is greater than interval.upper() + 1.
    // When first==last, interval does not merge with any elements, and we can use first as the insertion point,
    auto first = std::lower_bound(_intervals.begin(), _intervals.end(), interval,
                                  [](const Interval<T> &e, const Interval<T> &i) { return too_low_to_merge(e, i); });
    auto last = std::upper_bound(first, _intervals.end(), interval,
                                 [](const Interval<T> &i, const Interval<T> &e) { return too_high_to_merge(e, i); });
    // first == last is analogous to begin() == end(); there are no intervals to merge with.
    if (first == last) {
      _intervals.insert(first, interval);
      return;
    }
    // Non-empty, which means we need to combine all of [first, last) with interval.
    // Remove all items in [first+1, last) from the queue, and update [first] in place.
    Interval<T> merged{std::min(interval.lower(), first->lower()),
                       std::max(interval.upper(), std::prev(last)->upper())};
    *first = merged;
    _intervals.erase(std::next(first), last);
  }

  bool contains(T value) const {
    // Due to sorting, the first interval whose upper endpoint reaches value is the only one which could contain it.
    auto it = std::partition_point(_intervals.begin(), _intervals.end(),
                                   [value](const Interval<T> &e) { return e.upper() < value; });
    return it != _intervals.end() && pepp::core::contains(*it, value);
  }
  const auto &intervals() const { return _intervals; }
  // Keeps the allocation, so a set refilled every repaint stops allocating once it reaches its high-water mark.
  void clear() { _intervals.clear(); }

private:
  // Avoid +/-1 below to avoid overflow at T's extremes.

  // Return true if e is less than i AND there is at least one T between them.
  static constexpr bool too_low_to_merge(const Interval<T> &e, const Interval<T> &i) {
    return e.upper() < i.lower() && T(i.lower() - e.upper()) > T(1);
  }
  // Return true if e is greate than i AND there is at least one T between them
  static constexpr bool too_high_to_merge(const Interval<T> &e, const Interval<T> &i) {
    return e.lower() > i.upper() && T(e.lower() - i.upper()) > T(1);
  }
};

template <typename T> std::ostream &operator<<(std::ostream &os, const IntervalSet<T> &set) {
  for (const auto &i : set.intervals()) os << i;
  return os;
}
} // namespace pepp::core
