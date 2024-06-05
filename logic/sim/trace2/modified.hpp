#pragma once
#include <ostream>
#include <set>
#include "bits/mask.hpp"
#include "packet_utils.hpp"
#include "sim/api2.hpp"

namespace sim::trace2 {
template <typename T> struct Interval {
  explicit Interval() : _lower(T()), _upper(T()) {}
  explicit Interval(T point) : _lower(point), _upper(point) {}
  // Enforce lower <= upper to make math easier.
  Interval(T lower, T upper) : _lower(lower), _upper(upper) { Q_ASSERT(lower <= upper); }
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
  void insert(T lower, T upper) { insert(Interval<T>(lower, upper)); }
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

template <typename addr_size_t> class ModifiedAddressSink : public ::sim::api2::trace::Sink {
public:
  virtual ~ModifiedAddressSink() = default;
  bool analyze(api2::trace::PacketIterator iter, sim::api2::trace::Direction) override {
    using namespace sim::api2::packet;
    auto startAddrBytes = sim::trace2::get_address_bytes(*iter);
    if (!startAddrBytes) {
    } else if (auto len = sim::trace2::packet_payloads_length(iter, false); len > 0) {
      addr_size_t start = startAddrBytes->to_address<addr_size_t>();
      // If the number of bytes to represent the address is large than our address type, or the address type
      // will not fit int the u64, modular arithmetic will not work. Depend on typecast to truncate.
      uint64_t endAsU64 = ((static_cast<uint64_t>(start)) + static_cast<uint64_t>(len) - 1ull);
      // Use bitmask to force wraparound when len < sizeof(addr_size_t).
      addr_size_t end = endAsU64 & bits::mask(startAddrBytes->len);
      // Split in to upper and lower intervals.
      if (end < start) {
        addr_size_t maxAddr = (1ull << (startAddrBytes->len * 8)) - 1;
        _iset.insert(start, maxAddr);
        _iset.insert(0, end);
      } else
        _iset.insert(start, end);
    }
    return true;
  }
  void clear() { _iset.clear(); }
  const std::set<Interval<addr_size_t>> &intervals() const { return _iset.intervals(); }
  bool contains(addr_size_t addr) const {
    auto i = _iset.intervals();
    if (i.size() == 0)
      return false;
    // Use O(lg n) search to find glb.
    // If glb is at the start, this is the only interval which could contain addr.
    else if (auto lb = i.lower_bound(Interval<addr_size_t>{addr, addr}); lb == i.cbegin())
      return sim::trace2::contains(*lb, addr);
    else if (lb != i.cend() && lb->lower() <= addr)
      return sim::trace2::contains(*lb, addr);
    // prev might fail if lb is cbegin.
    else if (auto prev = std::prev(lb); prev == i.cend())
      return false;
    else
      return sim::trace2::contains(*prev, addr);
  }
  void insert(addr_size_t addr) { _iset.insert(addr); }
  void insert(addr_size_t lower, addr_size_t upper) { _iset.insert(lower, upper); }

private:
  IntervalSet<addr_size_t> _iset;
};
} // namespace sim::trace2
