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
#include "sim3/api/traced/memory_target.hpp"
#include "sim3/api/traced/trace_endpoint.hpp"
#include "sim3/trace/modified.hpp"

namespace sim::memory {
template <typename Address>
class SimpleBus : public api2::memory::Target<Address>,
                  public api2::memory::Translator<Address>,
                  public sim::api2::trace::Source {
public:
  using AddressSpan = typename api2::memory::AddressSpan<Address>;
  SimpleBus(api2::device::Descriptor device, AddressSpan span);
  ~SimpleBus() = default;
  SimpleBus(SimpleBus &&other) noexcept = default;
  SimpleBus &operator=(SimpleBus &&other) = default;
  // Disable copy construction and assignment, since it would be incorrect for
  // multiple objects to share a device descriptor.
  SimpleBus(const SimpleBus &) = delete;
  SimpleBus &operator=(const SimpleBus &) = delete;

  // Target interface
  sim::api2::device::ID deviceID() const override { return _device.id; }
  sim::api2::device::Descriptor device() const override { return _device; }
  AddressSpan span() const override;
  api2::memory::Result read(Address address, bits::span<quint8> dest, api2::memory::Operation op) const override;
  api2::memory::Result write(Address address, bits::span<const quint8> src, api2::memory::Operation op) override;
  void clear(quint8 fill) override;
  void dump(bits::span<quint8> dest) const override;

  // Translator interface
  std::tuple<bool, sim::api2::device::ID, Address> forward(Address address) const override;
  std::optional<Address> backward(sim::api2::device::ID child, Address address) const override;
  void setPathManager(QSharedPointer<api2::Paths> paths) override { _paths = paths; };
  QSharedPointer<const api2::Paths> pathManager() const override { return _paths; }

  // Source interface
  void setBuffer(api2::trace::Buffer *tb) override;
  const api2::trace::Buffer *buffer() const override { return _tb; }
  // Overload so that we can get mutable buffer when creating future memory devices.
  api2::trace::Buffer *buffer() { return _tb; }
  void trace(bool enabled) override;

  // Bus API
  void pushFrontTarget(AddressSpan at, api2::memory::Target<Address> *target);
  api2::memory::Target<Address> *deviceAt(Address address);
  void removeAllTargets();

private:
  const api2::memory::Target<Address> *device(sim::api2::device::ID id) const {
    auto it = std::lower_bound(_devices.cbegin(), _devices.cend(), id, LBID{});
    if (it == _devices.cend() || it->first != id) return nullptr;
    return it->second;
  }
  api2::memory::Target<Address> *device(sim::api2::device::ID id) {
    auto it = std::lower_bound(_devices.begin(), _devices.end(), id, LBID{});
    if (it == _devices.end() || it->first != id) return nullptr;
    return it->second;
  }
  using TargetPair = std::pair<sim::api2::device::ID, api2::memory::Target<Address> *>;
  struct LBID {
    inline bool operator()(const TargetPair &V, const quint16 find) const { return V.first < find; }
  };

  AddressSpan _span;
  api2::device::Descriptor _device;
  sim::trace2::AddressBiMap<Address, quint16> _addrs;
  QVector<TargetPair> _devices;
  QSharedPointer<sim::api2::Paths> _paths = nullptr;
  mutable api2::trace::Buffer *_tb = nullptr;
  api2::trace::PathGuard makeGuard() const {
    if (!_tb || !_paths) return api2::trace::PathGuard(nullptr, -1);
    auto currentPath = _tb->currentPath();
    return api2::trace::PathGuard(_tb, _paths->find(currentPath, _device.id));
  }
};

template <typename Address>
SimpleBus<Address>::SimpleBus(api2::device::Descriptor device, AddressSpan span) : _span(span), _device(device) {}

template <typename Address> typename SimpleBus<Address>::AddressSpan SimpleBus<Address>::span() const { return _span; }

template <typename Address>
api2::memory::Result SimpleBus<Address>::read(Address address, bits::span<quint8> dest,
                                              api2::memory::Operation op) const {
  using sim::api2::memory::convert;
  // TODO: add trace code that we traversed the bus.
  using E = api2::memory::Error;
  using T = std::tuple<Address, std::size_t>;
  // Length is 1-indexed, address are 0, so must offset by -1.
  if (auto maxDestAddr = (address + std::max<Address>(0, dest.size() - 1));
      address < _span.lower() || maxDestAddr > _span.upper())
    throw E(E::Type::OOBAccess, address);

  auto guard = makeGuard();
  // Construct a guard for the trace buffer.
  for (auto [offset, length] = T{0, dest.size()}; length > 0;) {
    auto region = _addrs.region_at(address + offset);
    if (!region) throw E(E::Type::Unmapped, address + offset);
    // Avoid nullptr check. If region is non-null and device is null, a class invariant was violated.
    auto dev = device(region->device);
    // Compute how many bytes we can read without OOB'ing on the device.
    auto devSpan = dev->span();
    auto usableLength = std::min<qsizetype>(length, size_inclusive(devSpan));
    // Convert bus address => device address
    auto busToDev = convert<Address>(address + offset, region->from, region->to);
    api2::memory::Result acc;
    acc = dev->read(busToDev, dest.subspan(offset, usableLength), op);

    offset += usableLength;
    length -= usableLength;
  }
  return {};
}
template <typename Address>
api2::memory::Result SimpleBus<Address>::write(Address address, bits::span<const quint8> src,
                                               api2::memory::Operation op) {
  using sim::api2::memory::convert;
  using sim::api2::memory::size;
  // TODO: add trace code that we traversed the bus.
  using E = api2::memory::Error;
  using T = std::tuple<Address, std::size_t>;
  // Length is 1-indexed, address are 0, so must offset by -1.
  if (auto maxDestAddr = (address + std::max<Address>(0, src.size() - 1));
      address < _span.lower() || maxDestAddr > _span.upper())
    throw E(E::Type::OOBAccess, address);

  auto guard = makeGuard();
  for (auto [offset, length] = T{0, src.size()}; length > 0;) {
    auto region = _addrs.region_at(address + offset);
    if (!region) throw E(E::Type::Unmapped, address + offset);
    // Avoid nullptr check. If region is non-null and device is null, a class invariant was violated.
    auto dev = device(region->device);
    // Compute how many bytes we can read without OOB'ing on the device.
    auto devSpan = dev->span();
    auto usableLength = std::min<qsizetype>(length, size<Address, false>(devSpan));
    // Convert bus address => device address
    auto busToDev = convert<Address>(address + offset, region->from, region->to);
    api2::memory::Result acc;
    acc = dev->write(busToDev, src.subspan(offset, usableLength), op);
    offset += usableLength;
    length -= usableLength;
  }
  return {};
}

template <typename Address> void SimpleBus<Address>::clear(quint8 fill) {
  for (auto dev : _devices) dev.second->clear(fill);
}

template <typename Address> void SimpleBus<Address>::dump(bits::span<quint8> dest) const {
  if (dest.size() <= 0) throw std::logic_error("dump requires non-0 size");
  // Can't iterate devices directly, as this would not respect layering.
  for (auto &rit : _addrs.regions()) {
    auto dev = device(rit.device);
    if (!dev) continue;
    dev->dump(dest.subspan(rit.from.lower(), size<Address, false>(rit.from)));
  }
}

namespace detail {
template <typename Address> struct SortOnDeviceID {
  bool operator()(const std::pair<sim::api2::device::ID, api2::memory::Target<Address> *> &lhs,
                  const std::pair<sim::api2::device::ID, api2::memory::Target<Address> *> &rhs) const {
    return lhs.first < rhs.first;
  }
};
} // namespace detail
template <typename Address>
void SimpleBus<Address>::pushFrontTarget(AddressSpan at, api2::memory::Target<Address> *target) {
  sim::trace2::Interval<Address> from = at, to = target->span();
  if (device(target->deviceID()) != nullptr) throw std::logic_error("Device ID already in use");
  _addrs.insert_or_overwrite(from, to, target->deviceID(), 0);
  _devices.push_back({target->deviceID(), target});
  std::sort(_devices.begin(), _devices.end(), detail::SortOnDeviceID<Address>{});
}

template <typename Address> sim::api2::memory::Target<Address> *SimpleBus<Address>::deviceAt(Address address) {
  auto region = _addrs.region_at(address);
  if (!region) return nullptr;
  return _devices[region.value().device].second;
}

template <typename Address> void SimpleBus<Address>::removeAllTargets() {
  _addrs.clear();
  _devices.clear();
}

template <typename Address>
std::tuple<bool, api2::device::ID, Address> SimpleBus<Address>::forward(Address address) const {
  return _addrs.value(address);
}

template <typename Address>
std::optional<Address> SimpleBus<Address>::backward(api2::device::ID child, Address address) const {
  if (auto r = _addrs.key(child, address); std::get<0>(r)) return std::get<1>(r);
  return std::nullopt;
}

template <typename Address> inline void SimpleBus<Address>::setBuffer(api2::trace::Buffer *tb) {
  this->_tb = tb;
  for (auto dev : _devices)
    if (auto as_source = dynamic_cast<api2::trace::Source *>(dev.second); as_source != nullptr)
      as_source->setBuffer(tb);
}

template <typename Address> inline void SimpleBus<Address>::trace(bool enabled) {
  if (_tb) _tb->trace(_device.id, enabled);
  for (auto dev : _devices)
    if (auto as_source = dynamic_cast<api2::trace::Source *>(dev.second); as_source != nullptr)
      as_source->trace(enabled);
}

} // namespace sim::memory
