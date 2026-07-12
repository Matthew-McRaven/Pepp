#include "simplebus.hpp"
#include "core/sim/memory/errors.hpp"
#include "core/sim/system.hpp"
#include "core/sim/systemparser.hpp"

SimpleBus::SimpleBus(Configuration cfg) : _config(cfg) {}

void SimpleBus::initialize(System *sys) {
  for (auto &mapping : _config.mappings) {
    auto target_dev = sys->find_relative(mapping.target, _config.fullname);
    if (!target_dev) throw std::logic_error("SimpleBus::initialize: mapping target not found: " + mapping.target);
    if (auto as_target = dynamic_cast<Target *>(target_dev); as_target != nullptr) {
      auto target_span = pepp::core::Interval<u32>::from_point_size(mapping.target_offset,
                                                                    pepp::core::size_inclusive(mapping.source_span));
      _addrs.insert_or_overwrite(mapping.source_span, target_span, target_dev->id(), mapping.access);
      _devices[target_dev->id()] = as_target;
    } else {
      throw std::logic_error("SimpleBus::initialize: mapping target is not a Target: " + mapping.target);
    }
  }
}

const Device::Configuration &SimpleBus::config() const { return _config; }

const Device::ID SimpleBus::id() const { return _config.id; }

Device::Type SimpleBus::type() const {
  using namespace bits;
  return Device::Type::MemoryTarget | Device::Type::MemoryInitiator | Device::Type::Traceable;
}

std::unique_ptr<DeviceSerializer> SimpleBus::serializer() const { return make_serializer(); }

std::unique_ptr<DeviceSerializer> SimpleBus::make_serializer() { return nullptr; }

void SimpleBus::set_buffer(Buffer *tb) {
  _tb = tb;
  for (auto dev : _devices)
    if (auto as_source = dynamic_cast<Traceable *>(dev.second); as_source != nullptr) as_source->set_buffer(tb);
}

const Buffer *SimpleBus::buffer() const { return _tb; }

bool SimpleBus::can_generate_traces() const { return true; }

void SimpleBus::trace(bool enabled) {
  if (_tb) _tb->trace(id(), enabled);
}

bool SimpleBus::traced() const { return _tb != nullptr && _tb->traced(id()); }

AddressSpan SimpleBus::span() const { return _config.span; }

Target::Result SimpleBus::read(Address address, bits::span<u8> dst, Operation op) const {
  using E = Error;
  const auto span = _config.span;
  using T = std::tuple<Address, std::size_t>;
  // Length is 1-indexed, address are 0, so must offset by -1.
  if (auto max_addr = (address + std::max<Address>(0, dst.size() - 1));
      address < span.lower() || max_addr > span.upper())
    throw E(E::Type::OOBAccess, address);
  for (auto [offset, length] = T{0, dst.size()}; length > 0;) {
    auto region = _addrs.region_at(address + offset);
    if (!region) throw E(E::Type::Unmapped, address + offset);
    // Avoid nullptr check. If region is non-null and device is null, a class invariant was violated.
    auto dev = device(region->id);
    // Compute how many bytes we can read without OOB'ing on the device.
    auto usable_len = std::min<size_t>(length, pepp::core::size_inclusive(dev->span()));
    // Convert bus address => device address
    auto src = offset_map<Address>(address + offset, region->from, region->to);
    // TODO: stop ignoring the result of the write. If the device returns an error, we should propagate it.
    (void)dev->write(src, dst.subspan(offset, usable_len), op);
    offset += usable_len, length -= usable_len;
  }
  return {};
}

Target::Result SimpleBus::write(Address address, bits::span<const u8> src, Operation op) {
  using E = Error;
  const auto span = _config.span;
  using T = std::tuple<Address, std::size_t>;
  // Length is 1-indexed, address are 0, so must offset by -1.
  if (auto max_addr = (address + std::max<Address>(0, src.size() - 1));
      address < span.lower() || max_addr > span.upper())
    throw E(E::Type::OOBAccess, address);
  for (auto [offset, length] = T{0, src.size()}; length > 0;) {
    auto region = _addrs.region_at(address + offset);
    if (!region) throw E(E::Type::Unmapped, address + offset);
    // Avoid nullptr check. If region is non-null and device is null, a class invariant was violated.
    auto dev = device(region->id);
    // Compute how many bytes we can read without OOB'ing on the device.
    auto usable_len = std::min<size_t>(length, pepp::core::size_inclusive(dev->span()));
    // Convert bus address => device address
    auto dst = offset_map<Address>(address + offset, region->from, region->to);
    // TODO: stop ignoring the result of the write. If the device returns an error, we should propagate it.
    (void)dev->write(dst, src.subspan(offset, usable_len), op);
    offset += usable_len, length -= usable_len;
  }
  return {};
}

void SimpleBus::clear(u8 fill) {
  for (auto dev : _devices) dev.second->clear(fill);
  _config.fill = fill;
}

void SimpleBus::dump(bits::span<u8> dest) const { throw std::logic_error("SimpleBus::dump not implemented"); }

Target *SimpleBus::device(ID id) const {
  auto it = _devices.find(id);
  if (it != _devices.end()) return it->second;
  return nullptr;
}