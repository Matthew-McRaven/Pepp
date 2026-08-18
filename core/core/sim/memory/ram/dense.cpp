#include "dense.hpp"
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
  _data.resize(size_inclusive(_config.span), _config.fill);
}

std::span<const u8> Dense::data() const { return std::span<const u8>{_data.data(), std::size_t(_data.size())}; }

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

AddressSpan Dense::span() const { return _config.span; }

Target::Result Dense::read(Address address, bits::span<u8> dest, Operation op) const {
  using E = Error;
  const auto span = _config.span;
  // Length is 1-indexed, address are 0, so must offset by -1.
  const auto max_addr = (address + std::max<Address>(0, dest.size() - 1));
  if (address < span.lower() || max_addr > span.upper()) throw E(E::Type::OOBAccess, address);
  const auto offset = address - span.lower();
  const auto src = bits::span<const u8>{_data.data(), std::size_t(_data.size())}.subspan(offset);
  bits::memcpy(dest, src);
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
  auto dest = bits::span<u8>{_data.data(), std::size_t(_data.size())}.subspan(offset);
  _trace.emit_write(op, address, dest.first(src.size()), src);
  bits::memcpy(dest, src);
  if (is_performance_countable(op)) _counters.wr_bytes += src.size();
  return {};
}

Target::Result Dense::write_increment(Address address, bits::span<const u8> src, Operation op) {
  using E = Error;
  auto span = _config.span;
  // Length is 1-indexed, address are 0, so must offset by -1.
  const auto max_addr = (address + std::max<Address>(0, src.size() - 1));
  if (address < span.lower() || max_addr > span.upper()) throw E(E::Type::OOBAccess, address);
  const auto offset = address - span.lower();
  auto dest = bits::span<u8>{_data.data(), std::size_t(_data.size())}.subspan(offset);
  _trace.emit_write_increment(op, address, dest.first(src.size()), src);
  bits::memcpy(dest, src);
  if (is_performance_countable(op)) _counters.wr_bytes += src.size();
  return {};
}

void Dense::clear(u8 fill) {
  // TODO: emit a "clear" trace to TB.
  _config.fill = fill;
  std::fill(_data.begin(), _data.end(), fill);
}

void Dense::dump(bits::span<u8> dest) const {
  if (dest.size() <= 0) throw std::logic_error("dump requires non-0 size");
  bits::memcpy(dest, bits::span<const u8>{_data.data(), std::size_t(_data.size())});
}
