/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include "core/math/bitmanip/copy.hpp"
#include "core/sim/api/device.hpp"
#include "core/sim/api/memory.hpp"
#include "core/sim/api/trace.hpp"

// D type allows sticking custom data in a given node.
// As a class invariant, we ensure that no Nodes exist with overlapping from intervals.
template <typename D = u16> class AddressTranslationMap {
public:
  using Interval = pepp::core::Interval<u32>;
  // A single address translation. device indicates the device which owns the "to" address range.
  // Data is a custom tag to store additional information, like context IDs for address translation.
  struct Node {
    Device::ID id;
    Interval from, to;
    D data;
    // Ignore to, device, data since they are values, not keys.
    auto operator<=>(const Node &other) const { return from <=> other.from; }
  };
  // Create a node with the given address translation parameters. If the from interval overlaps with any existing nodes,
  // those nodes are shrunk to avoid overlap. If the from interval entirely contains any nodes, those nodes are removed.
  // PRE: from and to must be of the same length.
  void insert_or_overwrite(Interval from, Interval to, Device::ID device, D data) {
    using namespace pepp::core;
    assert((size(from) == size(to)));
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
        prev->to = {prev->to.lower(), u32(prev->to.lower() + size_exclusive(prev->from))};
      }
    }
    // UB is first element such that from < other
    if (auto ub = std::upper_bound(_elements.begin(), _elements.end(), from, AddressTranslationMap<D>::UBFrom{});
        ub != _elements.end()) {
      if (intersects(from, ub->from)) {
        ub->from = {from.upper(), ub->from.upper()};
        ub->to = {u32(ub->to.upper() - size_inclusive(ub->from)), ub->to.upper()};
      }
    }
    // Insert Node & resort elements;
    _elements.push_back({device, from, to, data});
    std::sort(_elements.begin(), _elements.end());
  }

  // Translate T in "from" space T in "to" space, also returning device.
  // Return {false, X, X} if no mapping is found.
  std::tuple<bool, Device::ID, u32> value(u32 from_key) const {
    auto region = region_at(from_key);
    if (region) return {true, region->device, offset_map(from_key, region->from, region->to)};
    return {false, Device::ID{0}, 0};
  }

  // Given an address in the "from" space, return the associated address translation.
  std::optional<Node> region_at(u32 from_key) const {
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
    bool operator()(const Node &V, const Interval &find) const { return V.from < find; }
    bool operator()(const Node &V, const u32 &find) const { return V.from.lower() < find; }
  };
  struct UBFrom {
    bool operator()(const Interval &find, const Node &V) const { return find < V.from; }
  };
  // Must always be sorted to allow log(n) forward translations.
  std::vector<Node> _elements;
};

class SimpleBus final : public Device, public Target, public Initiator, public Traceable {
public:
  static const inline std::string compatible = "ram,dense";
  struct Configuration : public Device::Configuration {
    u8 fill{0};
    AddressSpan span;
    FailPolicy fail_policy = FailPolicy::RaiseError;

    struct Mapping {
      enum Access : u8 { Read = 1 << 0, Write = 1 << 1, Execute = 1 << 2 };
      std::string target;
      Access access = (Access)(Access::Read | Access::Write | Access::Execute);
      AddressSpan source_span;
      Address target_offset;
    };
    std::vector<Mapping> mappings;
  };
  SimpleBus(Configuration config);
  ~SimpleBus() = default;
  SimpleBus(SimpleBus &&other) noexcept = default;
  SimpleBus &operator=(SimpleBus &&other) = default;
  // Disable copy construction and assignment, since it would be incorrect for
  // multiple objects to share a device descriptor.
  SimpleBus(const SimpleBus &) = delete;
  SimpleBus &operator=(const SimpleBus &) = delete;

  // Device interface
  void initialize(System *) override;
  const Device::Configuration &config() const override;
  const Device::ID id() const override;
  Device::Type type() const override;
  std::unique_ptr<DeviceSerializer> serializer() const override;
  static std::unique_ptr<DeviceSerializer> make_serializer();

  // Traceable interface
  void set_buffer(Buffer *tb) override;
  const Buffer *buffer() const override;
  bool can_generate_traces() const override;
  void trace(bool enabled) override;
  bool traced() const override;

  // Target interface
  AddressSpan span() const override;
  Result read(Address address, bits::span<u8> dest, Operation op) const override;
  Result write(Address address, bits::span<const u8> src, Operation op) override;
  void clear(u8 fill) override;
  void dump(bits::span<u8> dest) const override;

private:
  Target *device(Device::ID id) const;

  Configuration _config;
  AddressTranslationMap<Configuration::Mapping::Access> _addrs;
  std::unordered_map<Device::ID, Target *> _devices;
  Buffer *_tb = nullptr;
};
