/*
 * /Copyright (c) 2023-2025. Stanley Warford, Matthew McRaven
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
#include <ostream>
#include <set>
#include "./packet_utils.hpp"
#include "core/libs/bitmanip/mask.hpp"
#include "sim3/api/memory_address.hpp"
#include "sim3/api/traced/memory_path.hpp"
#include "sim3/api/traced/memory_target.hpp"
#include "sim3/api/traced/trace_endpoint.hpp"
namespace sim::trace2 {
template <typename T> using Interval = sim::api2::memory::Interval<T>;

// Class to store and merge intervals of numeric types.
// Good words to google: interval tree, interval set.
// BUG: boundary arithmetic can overflow, so require unsigned to avoid UB.
template <std::unsigned_integral T, bool right_inclusive> class IntervalSet {
  std::set<Interval<T>> _intervals;

public:
  void insert(T lower, T upper) { insert(Interval<T>(lower, upper)); }
  void insert(T point) { insert(Interval<T>(point)); }
  void insert(Interval<T> interval) {
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
      } else if (contains(*prev, interval))
        return; // Optimization to avoid processing an insert / merge when containment is met.
      else if (intersects(*prev, interval) || prev->upper() + offset == interval.lower()) {
        bool _t = intersects(*prev, interval);
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
  const std::set<Interval<T>> &intervals() const { return _intervals; }
  void clear() { _intervals.clear(); }
};

template <typename T, bool right_inclusive>
std::ostream &operator<<(std::ostream &os, const IntervalSet<T, right_inclusive> &set) {
  for (const auto &i : set.intervals()) os << i;
  return os;
}

// Each device must only appear in the map once. D type allows sticking custom data in a given node.
// As a class invariant, we ensure that no Nodes exist with overlapping from intervals.
template <std::unsigned_integral T, typename D = quint16> class AddressBiMap {
public:
  // A single address translation. device indicates the device which owns the "to" address range.
  // Data is a custom tag to store additional information, like context IDs for address translation.
  struct Node {
    api2::device::ID device;
    Interval<T> from, to;
    D data;
    // Ignore device, data since they are values, not keys.
    auto operator<=>(const Node &other) const {
      if (auto cmp_from = from <=> other.from; cmp_from != 0) return cmp_from;
      return to <=> other.to;
    }
  };
  // Create a node with the given address translation parameters. If the from interval overlaps with any existing nodes,
  // those nodes are shrunk to avoid overlap. If the from interval entirely contains any nodes, those nodes are removed.
  // PRE: from and to must be of the same length.
  // PRE: device must not already be in the map.
  void insert_or_overwrite(Interval<T> from, Interval<T> to, sim::api2::device::ID device, D data) {
    Q_ASSERT((size<T, true>(from) == size<T, true>(to)));
    // Erase any nodes entirely contained within "from".
    using pepp::core::contains;
    if (auto remove =
            std::remove_if(_elements.begin(), _elements.end(), [&from](auto &n) { return contains(from, n.from); });
        remove != _elements.end())
      _elements.erase(remove, _elements.end());

    // Shrink any intervals that intersect from.
    // We only need to adjust the immediately adjacent elements, since no elements overlap because of above erase.
    // We find the lower_bound of from, which is the first element that is greater or equal to from.
    // Any non-sentinel case implies that we need to evaluate the previous element for its upper bound
    if (auto lb = std::lower_bound(_elements.begin(), _elements.end(), from, LBFrom{});
        lb != _elements.end() && lb != _elements.begin()) {
      auto prev = std::prev(lb);
      if (prev != _elements.end() && intersects(from, prev->from)) {
        prev->from = {prev->from.lower(), from.lower()};
        prev->to = {prev->to.lower(), T(prev->to.lower() + size<T, true>(prev->from))};
      }
    }
    // UB is first element such that from < other
    if (auto ub = std::upper_bound(_elements.begin(), _elements.end(), from, AddressBiMap<T, D>::UBFrom{});
        ub != _elements.end()) {
      if (intersects(from, ub->from)) {
        ub->from = {from.upper(), ub->from.upper()};
        ub->to = {T(ub->to.upper() - size<T, true>(ub->from)), ub->to.upper()};
      }
    }
    // Insert Node & resort elements;
    _elements.push_back({device, from, to, data});
    std::sort(_elements.begin(), _elements.end());
  }

  // Translate T in "from" space T in "to" space, also returning device.
  // Return {false, 0, X} if no mapping is found.
  std::tuple<bool, sim::api2::device::ID, T> value(T from_key) const {
    auto region = region_at(from_key);
    if (region) return {true, region->device, convert(from_key, region->from, region->to)};
    return {false, 0, 0};
  }

  // Translate from T in "to" space to T in "from" space. Use data tag to filter valid "to" intervals.
  std::tuple<bool, T> key(sim::api2::device::ID device, T to_value) const {
    // Elements are not sorted by "to" interval, so we must do a full linear search.
    for (const auto &node : _elements)
      if (node.device == device && contains(node.to, to_value)) return {true, convert(to_value, node.to, node.from)};
    return {false, T()};
  }

  // Given an address in the "from" space, return the region it belongs to.
  std::optional<Node> region_at(T from_key) const {
    using pepp::core::contains;
    if (_elements.size() == 0) return std::nullopt;
    // Use O(lg n) search to find glb.
    // If glb is at the start, this is the only interval which could contain addr.
    else if (auto lb = std::lower_bound(_elements.cbegin(), _elements.cend(), from_key, LBFrom{});
             // If at end, deref will cause OOB access.
             lb != _elements.cend() &&
             // If at begin, we can only check this element. lb may also return a lb whose lower == from_key
             ((lb == _elements.cbegin() && contains(lb->from, from_key)) || lb->from.lower() <= from_key))
      return *lb;
    // Otherwise lb might point to an element > from_key; so go to previous.
    else if (auto prev = std::prev(lb); prev != _elements.cend() && contains(prev->from, from_key)) return *prev;
    return std::nullopt;
  }
  const std::span<const Node> regions() const { return _elements; }
  void clear() { _elements.clear(); }

private:
  struct LBFrom {
    bool operator()(const Node &V, const Interval<T> &find) const { return V.from < find; }
    bool operator()(const Node &V, const T &find) const { return V.from.lower() < find; }
  };
  struct UBFrom {
    bool operator()(const Interval<T> &find, const Node &V) const { return find < V.from; }
  };
  // Must always be sorted to allow log(n) forward translations.
  std::vector<Node> _elements;
};

template <typename Address> class ModifiedAddressSink : public ::sim::api2::trace::Sink {
public:
  virtual ~ModifiedAddressSink() = default;
  bool analyze(api2::trace::PacketIterator iter, sim::api2::trace::Direction) override {
    using namespace sim::api2::packet;
    auto startAddrBytes = sim::trace2::get_address_bytes(*iter);
    auto path = sim::trace2::get_path(*iter).value_or(0);
    auto id = sim::trace2::get_id(*iter).value_or(0);
    if (!startAddrBytes) {
    } else if (auto len = sim::trace2::packet_payloads_length(iter, false); len > 0) {
      Address start = startAddrBytes->to_address<Address>();
      // If the number of bytes to represent the address is large than our address type, or the address type
      // will not fit int the u64, modular arithmetic will not work. Depend on typecast to truncate.
      uint64_t endAsU64 = ((static_cast<uint64_t>(start)) + static_cast<uint64_t>(len) - 1ull);
      // Use bitmask to force wraparound when len < sizeof(addr_size_t).
      Address end = endAsU64 & bits::mask(startAddrBytes->len);
      start = translate(id, path, start);
      end = translate(id, path, end);
      // Split in to upper and lower intervals.
      if (end < start) {
        // TODO: This is probably wrong when intermediate addresses have more bytes.
        Address maxAddr = (1ull << (startAddrBytes->len * 8)) - 1;
        _iset.insert(start, maxAddr);
        _iset.insert(0, end);
      } else _iset.insert(start, end);
    }
    return true;
  }
  void clear() { _iset.clear(); }
  const std::set<Interval<Address>> &intervals() const { return _iset.intervals(); }
  bool contains(Address addr) const {
    using pepp::core::contains;
    auto i = _iset.intervals();
    if (i.size() == 0) return false;
    // Use O(lg n) search to find glb.
    // If glb is at the start, this is the only interval which could contain addr.
    else if (auto lb = i.lower_bound(Interval<Address>{addr, addr}); lb == i.cbegin()) return contains(*lb, addr);
    else if (lb != i.cend() && lb->lower() <= addr) return contains(*lb, addr);
    // prev might fail if lb is cbegin.
    else if (auto prev = std::prev(lb); prev == i.cend()) return false;
    else return contains(*prev, addr);
  }

protected:
  using path_t = api2::packet::path_t;
  using device_id_t = api2::device::ID;
  virtual Address translate(device_id_t, path_t, Address addr) const { return addr; }

private:
  IntervalSet<Address, true> _iset;
};

template <typename Address> class TranslatingModifiedAddressSink : public ModifiedAddressSink<Address> {
public:
  TranslatingModifiedAddressSink(QSharedPointer<const api2::Paths> paths,
                                 const api2::memory::Translator<Address> *translator)
      : ModifiedAddressSink<Address>(), _translator(translator), _paths(paths) {}

protected:
  using path_t = api2::packet::path_t;
  using device_id_t = api2::device::ID;
  Address translate(device_id_t dev, path_t, Address addr) const override {
    return _translator->backward(dev, addr).value_or(0);
  }

private:
  const api2::memory::Translator<Address> *_translator = nullptr;
  QSharedPointer<const api2::Paths> _paths = nullptr;
};
} // namespace sim::trace2
