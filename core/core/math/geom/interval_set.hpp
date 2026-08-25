#pragma once

#include <set>
#include "core/math/geom/interval.hpp"
namespace pepp::core {

// Class to store and merge intervals of numeric types.
// Good words to google: interval tree, interval set.
// BUG: boundary arithmetic can overflow, so require unsigned to avoid UB.
template <std::unsigned_integral T, bool right_inclusive> class IntervalSet {
  std::set<Interval<T>> _intervals;

public:
  void insert(T lower, T upper) { insert(Interval<T>(lower, upper)); }
  void insert(T point) { insert(Interval<T>(point)); }
  void insert(Interval<T> interval) {
    if (!interval.valid()) return;
    static constexpr T offset = right_inclusive ? T(1) : T(0);
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
      } else if (pepp::core::contains(*prev, interval))
        return; // Optimization to avoid processing an insert / merge when containment is met.
      else if (intersects(*prev, interval) || prev->upper() + offset == interval.lower()) {
        interval = {prev->lower(), interval.upper()};
        // prev->upper <= interval.upper due to lower_bound.
        // Start the merge process from the previous interval, eliminating an extra erase call.
        eraseStart = eraseEnd = prev;
      }
    }

    // Merge with following intervals.
    for (auto it = eraseStart;
         it != _intervals.end() && (intersects(*it, interval) || it->lower() == interval.upper() + offset);
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
  // Check if a value is contained in any of the intervals using a binary search.
  bool contains(T value) const {
    if (_intervals.size() == 0) return false;
    // Use O(lg n) search to find glb.
    // If glb is at the start, this is the only interval which could contain addr.
    else if (auto lb = _intervals.lower_bound(Interval<T>(value)); lb != _intervals.cend() && lb->lower() == value)
      return pepp::core::contains<T>(*lb, value);
    else if (lb == _intervals.cbegin()) return false;
    else return pepp::core::contains<T>(*std::prev(lb), value);
  }
  const std::set<Interval<T>> &intervals() const { return _intervals; }
  void clear() { _intervals.clear(); }
};

template <typename T, bool right_inclusive>
std::ostream &operator<<(std::ostream &os, const IntervalSet<T, right_inclusive> &set) {
  for (const auto &i : set.intervals()) os << i;
  return os;
}
} // namespace pepp::core