#include "dense.hpp"
#include <cstring>
#include <nlohmann/json.hpp>
#include "core/sim/memory/errors.hpp"
#include "core/sim/system.hpp"
#include "core/sim/systemparser.hpp"

namespace {
Device *create_dense(const nlohmann::json &self, System *sys, Device *par) {
  Dense::Configuration cfg;
  try {
    parse_standard_fields(self, cfg);
    if (cfg.basename.empty()) throw ParsingError("RAM must have a basename");
    if (!self.contains("min_offset") || self["min_offset"].is_null()) throw ParsingError("RAM must have a min_offset");
    auto min = as_u32(self["min_offset"]);
    if (!self.contains("max_offset") || self["max_offset"].is_null()) throw ParsingError("RAM must have a max_offset");
    auto max = as_u32(self["max_offset"]);
    cfg.span = AddressSpan{min, max};
    if (self.contains("fill") && !self["fill"].is_null()) cfg.fill = as_i8(self["fill"]);
  } catch (const nlohmann::json::type_error &e) {
    throw ParsingError("Failed to parse dense RAM: " + std::string(e.what()));
  }
  return sys->make_device<Dense>(par, cfg);
}
void prefill_dense(nlohmann::json &obj) {
  obj["compatible"] = Dense::compatible;
  obj["basename"];
  obj["min_offset"];
  obj["max_offset"];
  obj["fill"] = 0;
}
void serialize_dense(nlohmann::json &obj, const System *sys, const Device *self) {
  auto casted = dynamic_cast<const Dense *>(self);
  if (!casted) throw std::logic_error("serialize_dense called on non-Dense device");
  obj["compatible"] = Dense::compatible;
  obj["basename"] = casted->config().basename;
  obj["min_offset"] = casted->casted_config().span.lower();
  obj["max_offset"] = casted->casted_config().span.upper();
  if (casted->casted_config().fill != 0) obj["fill"] = casted->casted_config().fill;
}
} // namespace

Dense::Dense(Configuration config) : Device(), _config(config) {
  const auto size = size_inclusive(_config.span);
  _data.resize(size, _config.fill);
  _dirty.resize(size, false);
}

std::span<const u8> Dense::data() const { return std::span<const u8>{_data.data(), std::size_t(_data.size())}; }

void Dense::initialize(System *sys) {
  static const auto RO = RegisterScan::Register::Access::Read;
  using SR = RegisterScan::Register;
  auto scan = sys->register_scan();
  scan->expose(SR{.byte_width = sizeof(_counters.rd_bytes),
                  .guest_access = RO,
                  .restore_on_step_back = false,
                  .kind = SR::Kind::Count,
                  .visibility = SR::Visibility::Internal,
                  .target = id(),
                  .order = bits::hostOrder(),
                  .name = "rd_bytes",
                  .loc = &_counters.rd_bytes});
  scan->expose(SR{.byte_width = sizeof(_counters.wr_bytes),
                  .guest_access = RO,
                  .restore_on_step_back = false,
                  .kind = SR::Kind::Count,
                  .visibility = SR::Visibility::Internal,
                  .target = id(),
                  .order = bits::hostOrder(),
                  .name = "wr_bytes",
                  .loc = &_counters.wr_bytes});
}

void Dense::reset() {
  clear(_config.fill);
  _counters = {};
  _may_trace = true;
}

const Device::ID Dense::id() const { return _config.id; }

const Device::Configuration &Dense::config() const { return _config; }

const Dense::Configuration &Dense::casted_config() const { return _config; }

Device::Type Dense::type() const {
  using namespace bits;
  using T = Device::Type;
  return T::MemoryTarget | T::Traceable;
}

u64 Dense::features() const { return 0; }

std::unique_ptr<DeviceSerializer> Dense::serializer() const { return make_serializer(); }

std::unique_ptr<DeviceSerializer> Dense::make_serializer() {
  DeviceSerializer s{
      .parser = create_dense, .prefill = prefill_dense, .serialize = serialize_dense, .compatible = Dense::compatible};
  return std::make_unique<DeviceSerializer>(std::move(s));
}

void Dense::set_recorder(const trace::Recorder &recorder) { _trace = recorder; }

bool Dense::can_generate_traces() const { return true; }

void Dense::trace(bool enabled) { _trace.set_traced(enabled); }

bool Dense::traced() const { return _trace.traced(); }

void Dense::on_traced_changed(bool enabled) { _may_trace = enabled; }

AddressSpan Dense::span() const { return _config.span; }

Target::Result Dense::read(Address address, bits::span<u8> dest, Operation op) const {
  using E = Error;
  const auto span = _config.span;
  // Length is 1-indexed, address are 0, so must offset by -1.
  const auto max_addr = (address + std::max<Address>(0, dest.size() - 1));
  if (address < span.lower() || max_addr > span.upper()) throw E(E::Type::OOBAccess, address);
  const auto offset = address - span.lower();
  const u8 *src = _data.data() + offset;
  // Switched on the width so the copy length is a constant the compiler can turn into a register operation for
  // common register sizes rather than a call to C's memcpy
  switch (dest.size()) {
  case 1: dest[0] = src[0]; break;
  case 2: std::memcpy(dest.data(), src, 2); break;
  case 4: std::memcpy(dest.data(), src, 4); break;
  case 8: std::memcpy(dest.data(), src, 8); break;
  default: std::memcpy(dest.data(), src, dest.size()); break;
  }
  if (is_performance_countable(op)) _counters.rd_bytes += dest.size();
  return {};
}

Target::Result Dense::write(Address address, bits::span<const u8> src, Operation op) {
  using E = Error;
  auto span = _config.span;
  // Length is 1-indexed, address are 0, so must offset by -1.
  const auto max_addr = (address + std::max<Address>(0, src.size() - 1));
  if (address < span.lower() || max_addr > span.upper()) throw E(E::Type::OOBAccess, address);
  const auto offset = address - span.lower();
  u8 *dest = _data.data() + offset;
  if (_may_trace) _trace.emit_write(op, address, bits::span<const u8>{dest, src.size()}, src);
  // Switched on the width so the copy length is a constant the compiler can turn into a register operation for
  // common register sizes rather than a call to memcpy.
  switch (src.size()) {
  case 1:
    dest[0] = src[0];
    _dirty[offset] = true;
    break;
  case 2:
    std::memcpy(dest, src.data(), 2);
    _dirty[offset] = true, _dirty[offset + 1] = true;
    break;
  case 4:
    std::memcpy(dest, src.data(), 4);
    for (int i = 0; i < 4; i++) _dirty[offset + i] = true;
    break;
  case 8:
    std::memcpy(dest, src.data(), 8);
    for (int i = 0; i < 8; i++) _dirty[offset + i] = true;
    break;
  default:
    std::memcpy(dest, src.data(), src.size());
    std::fill(_dirty.begin() + offset, _dirty.begin() + offset + src.size(), true);
    break;
  }
  if (is_performance_countable(op)) _counters.wr_bytes += src.size();
  return {};
}

Target::Result Dense::write_increment(Address address, bits::span<const u8> src, Operation op, bits::Order order) {
  using E = Error;
  auto span = _config.span;
  // Length is 1-indexed, address are 0, so must offset by -1.
  const auto max_addr = (address + std::max<Address>(0, src.size() - 1));
  if (address < span.lower() || max_addr > span.upper()) throw E(E::Type::OOBAccess, address);
  const auto offset = address - span.lower();
  auto dest = bits::span<u8>{_data.data(), std::size_t(_data.size())}.subspan(offset);
  if (_may_trace) _trace.emit_write_increment(op, address, dest.first(src.size()), src, order);
  bits::memcpy(dest, src);
  std::fill(_dirty.begin() + offset, _dirty.begin() + offset + src.size(), true);
  if (is_performance_countable(op)) _counters.wr_bytes += src.size();
  return {};
}

void Dense::clear(u8 fill) {
  // TODO: emit a "clear" trace to TB.
  std::fill(_data.begin(), _data.end(), fill);
  std::fill(_dirty.begin(), _dirty.end(), false);
}

void Dense::dump(bits::span<u8> dest) const {
  if (dest.size() <= 0) throw std::logic_error("dump requires non-0 size");
  bits::memcpy(dest, bits::span<const u8>{_data.data(), std::size_t(_data.size())});
}

void Dense::collect_changes(pepp::core::IntervalSet<Address> &changed) const {
  // _dirty is indexed by offset, but the API is specified in addresses
  const auto base = _config.span.lower();
  const auto size = _dirty.size();
  for (std::size_t i = 0; i < size;) {
    if (!_dirty[i]) {
      ++i;
      continue;
    }
    const std::size_t start = i;
    while (i < size && _dirty[i]) ++i;
    // Runs are emitted low-to-high, which is the order IntervalSet prefers.
    changed.insert(static_cast<Address>(base + start), static_cast<Address>(base + i - 1));
  }
}

void Dense::clear_changes() { std::fill(_dirty.begin(), _dirty.end(), false); }
