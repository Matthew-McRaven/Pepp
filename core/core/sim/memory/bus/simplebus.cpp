#include "simplebus.hpp"
#include <algorithm>
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
        m.source.span = parse_span(mapping, "source_");
        // Check for target offset, defaulting to 0 if not provided.
        if (!mapping.contains("target_offset") || mapping["target_offset"].is_null()) m.target_offset = 0;
        else m.target_offset = as_u32(mapping["target_offset"]);
        // Search for read/write/execute values
        if (mapping.contains("access") && !mapping["access"].is_null()) {
          auto access = bits::to_lower(mapping["access"].get<std::string>());
          m.source.access = Access::None;
          if (access.find("r") != std::string::npos) m.source.access |= Access::Read;
          if (access.find("w") != std::string::npos) m.source.access |= Access::Write;
          if (access.find("x") != std::string::npos) m.source.access |= Access::Execute;
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
  using namespace bits;
  obj["target"] = mapping.target;
  obj["source_min_offset"] = mapping.source.span.lower();
  obj["source_max_offset"] = mapping.source.span.upper();
  obj["target_offset"] = mapping.target_offset;
  std::string access;
  if (any(mapping.source.access & Access::Read)) access += "r";
  if (any(mapping.source.access & Access::Write)) access += "w";
  if (any(mapping.source.access & Access::Execute)) access += "x";
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
                                                                    pepp::core::size_exclusive(mapping.source.span));
      _as_configured.insert_or_overwrite(mapping.source.span, target_span, target_dev->id(), mapping.source.access);
      _devices[target_dev->id()] = as_target;
    } else {
      throw std::logic_error("SimpleBus::initialize: mapping target is not a Target: " + mapping.target);
    }
  }
  _with_permission = _as_configured;
}

void SimpleBus::apply_permissions(std::span<const Permission> perms) {
  // Sort requested perms by address--matching order of _as_configured regions--to avoid O(n^2) search.
  std::vector<Permission> sorted(perms.begin(), perms.end());
  std::ranges::sort(sorted, {}, [](const Permission &perm) { return perm.span.lower(); });
  for (std::size_t it = 1; it < sorted.size(); it++)
    if (sorted[it].span.lower() <= sorted[it - 1].span.upper())
      throw std::invalid_argument("SimpleBus::apply_permissions: permissions must not overlap");
  _loaded = std::move(sorted);
  recompute_permissions();
}

void SimpleBus::recompute_permissions() {
  using namespace bits;
  const auto &sorted = _loaded;
  auto place = [&](auto &node, AddressSpan piece, Access access) {
    const AddressSpan to(offset_map(piece.lower(), node.from, node.to), offset_map(piece.upper(), node.from, node.to));
    _with_permission.insert_or_overwrite(piece, to, node.id, access);
  };
  _with_permission.clear();
  auto first = sorted.cbegin();
  for (const auto &node : _as_configured.regions()) {
    // Permissions entirely below this region cannot affect it or any later region.
    while (first != sorted.cend() && first->span.upper() < node.from.lower()) ++first;
    // The first address of this region not yet placed. u64 so it can hold the max value of Address(u32)+1.
    u64 cursor = node.from.lower();

    // Compute the intersection of the permissions with the current region.
    for (auto perm = first; perm != sorted.cend() && perm->span.lower() <= node.from.upper(); ++perm) {
      const auto overlap = pepp::core::intersection(node.from, perm->span);
      // Handle cases where there is a gap between the last permission and this one or when the first permission starts
      // inside the region. Then emit the overlap between the permission and region.
      if (cursor < overlap.lower()) place(node, AddressSpan(Address(cursor), overlap.lower() - 1), node.data);
      place(node, overlap, node.data & perm->access);
      cursor = u64(overlap.upper()) + 1;
    }
    // Handle the case where this region is not fully covered by any provided permission.
    if (cursor <= node.from.upper()) place(node, AddressSpan(Address(cursor), node.from.upper()), node.data);
  }
}

void SimpleBus::load(AddressSpan span, bits::span<const u8> data, Access access) {
  using E = Error;
  if (!span.valid()) return;
  else if (span.lower() < _config.span.lower() || span.upper() > _config.span.upper())
    throw E(E::Type::OOBAccess, span.lower());

  // Keep only the permissions which do not overlap the new span. Split spans which partially overlap.
  std::vector<Permission> kept;
  for (const auto &perm : _loaded) {
    if (!pepp::core::intersects(perm.span, span)) kept.push_back(perm);
    else {
      if (perm.span.lower() < span.lower())
        kept.push_back({AddressSpan(perm.span.lower(), span.lower() - 1), perm.access});
      if (perm.span.upper() > span.upper())
        kept.push_back({AddressSpan(span.upper() + 1, perm.span.upper()), perm.access});
    }
  }
  kept.push_back({span, access});
  std::ranges::sort(kept, {}, [](const Permission &perm) { return perm.span.lower(); });
  _loaded = std::move(kept);
  recompute_permissions();

  for (u64 at = span.lower(); at <= span.upper();) {
    auto region = _with_permission.region_at(static_cast<Address>(at));
    if (!region) throw E(E::Type::Unmapped, static_cast<Address>(at));
    const u64 end = std::min<u64>(span.upper(), region->from.upper());
    const auto piece = AddressSpan(offset_map<Address>(static_cast<Address>(at), region->from, region->to),
                                   offset_map<Address>(static_cast<Address>(end), region->from, region->to));
    const u64 offset = at - span.lower();
    const auto usable_len = offset >= data.size() ? 0 : std::min<u64>(end - at + 1, data.size() - offset);
    const auto effective_span = data.subspan(offset > data.size() ? data.size() : offset, usable_len);
    device(region->id)->load(piece, effective_span, access);
    at = end + 1;
  }
}

void SimpleBus::reset() {
  _loaded.clear();
  // Loaded permissions not part of power-on state.
  _with_permission = _as_configured;
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
    const Address at = address + offset;
    auto region = _with_permission.region_at(at);
    if (!region) throw E(E::Type::Unmapped, at);
    // Avoid nullptr check. If region is non-null and device is null, a class invariant was violated.
    auto dev = device(region->id);
    // Do not overflow this region. Device or permissions might change.
    const auto usable_len = std::min<u64>(length, u64(region->from.upper()) - at + 1);
    // Convert bus address => device address
    auto src = offset_map<Address>(at, region->from, region->to);
    // TODO: stop ignoring the result of the write. If the device returns an error, we should propagate it.
    (void)dev->read(src, dst.subspan(offset, usable_len), op);
    offset += usable_len, length -= usable_len;
  }
  return {};
}

Target::Result SimpleBus::write(Address address, bits::span<const u8> src, Operation op) {
  using namespace bits;
  using E = Error;
  const auto span = _config.span;
  using T = std::tuple<Address, std::size_t>;
  // Length is 1-indexed, address are 0, so must offset by -1.
  if (auto max_addr = (address + std::max<Address>(0, src.size() - 1));
      address < span.lower() || max_addr > span.upper())
    throw E(E::Type::OOBAccess, address);
  for (auto [offset, length] = T{0, src.size()}; length > 0;) {
    const Address at = address + offset;
    auto region = _with_permission.region_at(at);
    if (!region) throw E(E::Type::Unmapped, at);
    // Avoid nullptr check. If region is non-null and device is null, a class invariant was violated.
    auto dev = device(region->id);
    // Do not overflow this region. Device or permissions might change.
    const auto usable_len = std::min<u64>(length, u64(region->from.upper()) - at + 1);
    // Application and trace-replay writes (loaders, memory editors, step back) must be able to modify read-only memory.
    if (bits::any(region->data & Access::Write) ||
        (op.type == Operation::Type::Application || op.type == Operation::Type::BufferInternal)) {
      // Convert bus address => device address
      auto dst = offset_map<Address>(at, region->from, region->to);
      // TODO: stop ignoring the result of the write. If the device returns an error, we should propagate it.
      (void)dev->write(dst, src.subspan(offset, usable_len), op);
      offset += usable_len, length -= usable_len;
    } else if (_config.fail_policy == FailPolicy::YieldDefaultValue) offset += usable_len, length -= usable_len;
    else throw E(E::Type::WriteToRO, at);
  }
  return {};
}

void SimpleBus::clear(u8 fill) {
  for (auto dev : _devices) dev.second->clear(fill);
}

void SimpleBus::dump(bits::span<u8> dest) const { throw std::logic_error("SimpleBus::dump not implemented"); }

void SimpleBus::collect_changes(pepp::core::IntervalSet<Address> &changed) const {
  // No-op, since this bus owns no memory directly, and the debugger/system is responsible for walking the device tree.
}

void SimpleBus::clear_changes() {
  // No-op, see above.
}

Target *SimpleBus::device(ID id) const {
  auto it = _devices.find(id);
  if (it != _devices.end()) return it->second;
  return nullptr;
}