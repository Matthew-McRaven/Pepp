#include "simplebus.hpp"
#include <nlohmann/json.hpp>
#include "core/math/bitmanip/strings.hpp"
#include "core/sim/memory/errors.hpp"
#include "core/sim/system.hpp"
#include "core/sim/systemparser.hpp"

namespace {
AddressSpan parse_span(const nlohmann::json &obj, const std::string &prefix = "") {
  if (!obj.contains(prefix + "min_offset") || obj[prefix + "min_offset"].is_null())
    throw ParsingError("SimpleBus must have a min_offset");
  auto min = as_u32(obj[prefix + "min_offset"]);
  if (!obj.contains(prefix + "max_offset") || obj[prefix + "max_offset"].is_null())
    throw ParsingError("SimpleBus must have a max_offset");
  auto max = as_u32(obj[prefix + "max_offset"]);
  return AddressSpan{min, max};
}
Device *create_simplebus(const nlohmann::json &self, System *sys, Device *par) {
  using namespace bits;
  SimpleBus::Configuration cfg;
  using Access = SimpleBus::Configuration::Mapping::Access;
  try {
    parse_standard_fields(self, cfg);
    if (cfg.basename.empty()) throw ParsingError("SimpleBus must have a basename");
    cfg.span = parse_span(self);

    if (self.contains("fill") && !self["fill"].is_null()) cfg.fill = as_i8(self["fill"]);
    if (self.contains("fail_policy") && !self["fail_policy"].is_null()) {
      auto policy = bits::to_lower(self["fail_policy"].get<std::string>());
      if (policy == "raise_error") cfg.fail_policy = FailPolicy::RaiseError;
      else if (policy == "yield_default") cfg.fail_policy = FailPolicy::YieldDefaultValue;
      else throw ParsingError("Unknown fail_policy: " + policy);
    }
    if (self.contains("mappings") && self["mappings"].is_array()) {
      for (const auto &mapping : self["mappings"]) {
        SimpleBus::Configuration::Mapping m;
        if (!mapping.contains("target") || mapping["target"].is_null())
          throw ParsingError("SimpleBus mapping must have a string target");
        m.target = mapping["target"].get<std::string>();
        // Check for source min and max offset
        m.source_span = parse_span(mapping, "source_");
        // Check for target offset, defaulting to 0 if not provided.
        if (!mapping.contains("target_offset") || mapping["target_offset"].is_null()) m.target_offset = 0;
        m.target_offset = as_u32(mapping["target_offset"]);
        // Search for read/write/execute values
        if (mapping.contains("access") && !mapping["access"].is_null()) {
          auto access = bits::to_lower(mapping["access"].get<std::string>());
          m.access = Access::None;
          if (access.find("r") == std::string::npos) m.access |= Access::Read;
          if (access.find("w") == std::string::npos) m.access |= Access::Write;
          if (access.find("x") == std::string::npos) m.access |= Access::Execute;
        }
        cfg.mappings.push_back(m);
      }
    } else throw ParsingError("SimpleBus must have a mappings array");
  } catch (const nlohmann::json::type_error &e) {
    throw ParsingError("Failed to parse SimpleBus: " + std::string(e.what()));
  }
  return sys->make_device<SimpleBus>(par, cfg);
}
void prefill_simplebus(nlohmann::json &obj) {
  obj["compatible"] = SimpleBus::compatible;
  obj["basename"];
  obj["min_offset"];
  obj["max_offset"];
  obj["mappings"] = nlohmann::json::array();
  obj["fail_policy"] = "raise_error";
  obj["fill"] = 0;
}

void serialize_mapping(nlohmann::json &obj, const SimpleBus::Configuration::Mapping &mapping) {
  obj["target"] = mapping.target;
  obj["source_min_offset"] = mapping.source_span.lower();
  obj["source_max_offset"] = mapping.source_span.upper();
  obj["target_offset"] = mapping.target_offset;
  std::string access;
  if (mapping.access & SimpleBus::Configuration::Mapping::Access::Read) access += "r";
  if (mapping.access & SimpleBus::Configuration::Mapping::Access::Write) access += "w";
  if (mapping.access & SimpleBus::Configuration::Mapping::Access::Execute) access += "x";
  obj["access"] = access;
}

void serialize_simplebus(nlohmann::json &obj, const System *sys, const Device *self) {
  auto casted = dynamic_cast<const SimpleBus *>(self);
  if (!casted) throw std::logic_error("serialize_simplebus called on non-SimpleBus device");
  obj["compatible"] = SimpleBus::compatible;
  obj["basename"] = casted->config().basename;
  obj["min_offset"] = casted->casted_config().span.lower();
  obj["max_offset"] = casted->casted_config().span.upper();
  if (casted->casted_config().fill != 0) obj["fill"] = casted->casted_config().fill;
  switch (casted->casted_config().fail_policy) {
  case FailPolicy::RaiseError: break; // Default value; do not serialize.
  case FailPolicy::YieldDefaultValue: obj["fail_policy"] = "yield_default"; break;
  }
  nlohmann::json mappings = nlohmann::json::array();
  for (const auto &mapping : casted->mappings()) {
    nlohmann::json mapping_obj;
    serialize_mapping(mapping_obj, mapping);
    mappings.push_back(mapping_obj);
  }
  obj["mappings"] = mappings;
}

} // namespace

SimpleBus::SimpleBus(Configuration cfg) : _config(cfg) {}

const std::vector<SimpleBus::Configuration::Mapping> &SimpleBus::mappings() const { return _config.mappings; }

void SimpleBus::initialize(System *sys) {
  for (auto &mapping : _config.mappings) {
    auto target_dev = sys->find_relative(mapping.target, _config.fullname);
    if (!target_dev) throw std::logic_error("SimpleBus::initialize: mapping target not found: " + mapping.target);
    if (auto as_target = dynamic_cast<Target *>(target_dev); as_target != nullptr) {
      auto target_span = pepp::core::Interval<u32>::from_point_size(mapping.target_offset,
                                                                    pepp::core::size_exclusive(mapping.source_span));
      _addrs.insert_or_overwrite(mapping.source_span, target_span, target_dev->id(), mapping.access);
      _devices[target_dev->id()] = as_target;
    } else {
      throw std::logic_error("SimpleBus::initialize: mapping target is not a Target: " + mapping.target);
    }
  }
}

const Device::Configuration &SimpleBus::config() const { return _config; }

const SimpleBus::Configuration &SimpleBus::casted_config() const { return _config; }

const Device::ID SimpleBus::id() const { return _config.id; }

Device::Type SimpleBus::type() const {
  using namespace bits;
  return Device::Type::MemoryTarget | Device::Type::MemoryInitiator | Device::Type::Traceable;
}

std::unique_ptr<DeviceSerializer> SimpleBus::serializer() const { return make_serializer(); }

std::unique_ptr<DeviceSerializer> SimpleBus::make_serializer() {
  DeviceSerializer s{.parser = create_simplebus,
                     .prefill = prefill_simplebus,
                     .serialize = serialize_simplebus,
                     .compatible = SimpleBus::compatible};
  return std::make_unique<DeviceSerializer>(std::move(s));
}

// Don't pass this to children; they need their own device IDs.
// System will set all recorders as expected.
void SimpleBus::set_recorder(const trace::Recorder &recorder) { _trace = recorder; }

bool SimpleBus::can_generate_traces() const { return true; }

void SimpleBus::trace(bool enabled) { _trace.set_traced(enabled); }

bool SimpleBus::traced() const { return _trace.traced(); }

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
    (void)dev->read(src, dst.subspan(offset, usable_len), op);
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