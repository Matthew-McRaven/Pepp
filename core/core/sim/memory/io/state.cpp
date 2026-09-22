#include "core/sim/memory/io/state.hpp"
#include <nlohmann/json.hpp>
#include "core/math/bitmanip/copy.hpp"
#include "core/math/bitmanip/enums.hpp"
#include "core/sim/memory/errors.hpp"
#include "core/sim/system.hpp"
#include "core/sim/systemparser.hpp"

namespace {
Device *create_state(const nlohmann::json &self, System *sys, Device *par) {
  StateRegister::Configuration cfg;
  try {
    parse_standard_fields(self, cfg);
    if (cfg.basename.empty()) throw ParsingError("StateRegister must have a basename");
    if (!self.contains("offset") || self["offset"].is_null()) throw ParsingError("StateRegister must have an offset");
    const auto offset = as_u32(self["offset"]);
    u32 width = 1;
    if (self.contains("width") && !self["width"].is_null()) width = as_u32(self["width"]);
    if (width == 0 || width > 8) throw ParsingError("StateRegister width must be in [1, 8]");
    cfg.span = AddressSpan{offset, offset + width - 1};
    if (self.contains("fill") && !self["fill"].is_null()) cfg.fill = as_u8(self["fill"]);
  } catch (const nlohmann::json::type_error &e) {
    throw ParsingError("Failed to parse StateRegister: " + std::string(e.what()));
  }
  return sys->make_device<StateRegister>(par, cfg);
}

void prefill_state(nlohmann::json &obj) {
  obj["compatible"] = StateRegister::compatible;
  obj["basename"];
  obj["offset"];
  obj["width"] = 1;
  obj["fill"] = 0;
}

void serialize_state(nlohmann::json &obj, const System *, const Device *self) {
  auto casted = dynamic_cast<const StateRegister *>(self);
  if (!casted) throw std::logic_error("serialize_state called on non-StateRegister device");
  const auto &cfg = casted->casted_config();
  obj["compatible"] = StateRegister::compatible;
  obj["basename"] = cfg.basename;
  obj["offset"] = cfg.span.lower();
  obj["width"] = size_inclusive(cfg.span);
  if (cfg.fill != 0) obj["fill"] = cfg.fill;
}
} // namespace

StateRegister::StateRegister(Configuration config)
    : Device(), _config(config), _data(size_inclusive(config.span), config.fill) {}

bool StateRegister::changed() const { return _changed; }

void StateRegister::reset() { clear(_config.fill); }

const Device::Configuration &StateRegister::config() const { return _config; }

const StateRegister::Configuration &StateRegister::casted_config() const { return _config; }

const Device::ID StateRegister::id() const { return _config.id; }

Device::Type StateRegister::type() const {
  using namespace bits;
  using T = Device::Type;
  return T::MemoryTarget | T::Traceable | T::EventSource;
}

std::unique_ptr<DeviceSerializer> StateRegister::serializer() const { return make_serializer(); }

std::unique_ptr<DeviceSerializer> StateRegister::make_serializer() {
  DeviceSerializer s{.parser = create_state,
                     .prefill = prefill_state,
                     .serialize = serialize_state,
                     .compatible = StateRegister::compatible};
  return std::make_unique<DeviceSerializer>(std::move(s));
}

void StateRegister::set_recorder(const trace::Recorder &recorder) { _trace = recorder; }

bool StateRegister::can_generate_traces() const { return true; }

void StateRegister::trace(bool enabled) { _trace.set_traced(enabled); }

bool StateRegister::traced() const { return _trace.traced(); }

AddressSpan StateRegister::span() const { return _config.span; }

Target::Result StateRegister::read(Address address, bits::span<u8> dest, Operation) const {
  using E = Error;
  const auto span = _config.span;
  const auto max_addr = (address + std::max<Address>(0, dest.size() - 1));
  if (address < span.lower() || max_addr > span.upper()) throw E(E::Type::OOBAccess, address);
  bits::memcpy(dest, bits::span<const u8>{_data}.subspan(address - span.lower(), dest.size()));
  return {};
}

Target::Result StateRegister::write(Address address, bits::span<const u8> src, Operation op) {
  using E = Error;
  const auto span = _config.span;
  const auto max_addr = (address + std::max<Address>(0, src.size() - 1));
  if (address < span.lower() || max_addr > span.upper()) throw E(E::Type::OOBAccess, address);
  const auto dest = bits::span<u8>{_data}.subspan(address - span.lower(), src.size());
  _trace.emit_write(op, address, dest, src);
  bits::memcpy(dest, src);
  // Loader, debugger, and trace replay writes change the value without acting as memory-mapped IO.
  if (op.type != Operation::Type::Standard) return {};
  _changed = true;
  raise(id(), MemoryWritten(this, AddressSpan{address, max_addr}, src));
  return {};
}

void StateRegister::clear(u8 fill) {
  std::fill(_data.begin(), _data.end(), fill);
  _changed = false;
}

void StateRegister::dump(bits::span<u8> dest) const {
  bits::memcpy(dest.first(std::min(dest.size(), _data.size())), bits::span<const u8>{_data});
}

void StateRegister::collect_changes(pepp::core::IntervalSet<Address> &changed) const {
  if (_changed) changed.insert(_config.span);
}

void StateRegister::clear_changes() { _changed = false; }
