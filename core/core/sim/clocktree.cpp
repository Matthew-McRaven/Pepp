#include "./clocktree.hpp"
#include <array>
#include <nlohmann/json.hpp>
#include "core/math/bitmanip/copy.hpp"
#include "core/math/bitmanip/enums.hpp"
#include "core/sim/systemparser.hpp"
#include "system.hpp"

namespace {
using namespace pepp;

bits::Order parse_order(const nlohmann::json &node) {
  if (!node.contains("order") || node["order"].is_null()) return bits::Order::LittleEndian;
  const auto order = node["order"].get<std::string>();
  if (order == "little") return bits::Order::LittleEndian;
  else if (order == "big") return bits::Order::BigEndian;
  else throw ParsingError("Clock enable order must be one of: little, big");
}

std::optional<ClockEnable::Configuration> parse_enable(const nlohmann::json &self) {
  if (!self.contains("enable") || self["enable"].is_null()) return std::nullopt;
  const auto &node = self["enable"];
  if (!node.is_object()) throw ParsingError("Clock enable must be an object");
  const auto has = [&](const char *key) { return node.contains(key) && !node[key].is_null(); };
  ClockEnable::Configuration cfg;
  if (!has("source")) throw ParsingError("Clock enable must have a source");
  cfg.source = node["source"].get<std::string>();
  if (has("width") && !has("offset")) throw ParsingError("Clock enable width requires an offset");
  if (has("offset")) {
    const Address offset = as_u32(node["offset"]);
    const u8 width = has("width") ? as_u8(node["width"]) : 1;
    if (width == 0 || width > 8) throw ParsingError("Clock enable width must be in [1, 8]");
    if (offset > std::numeric_limits<Address>::max() - (width - 1))
      throw ParsingError("Clock enable offset + width overflows the address space");
    cfg.span = AddressSpan(offset, offset + width - 1);
  }

  const auto mode = has("mode") ? node["mode"].get<std::string>() : "";
  if (mode == "disable_on_write") {
    if (has("mask") || has("match") || has("order"))
      throw ParsingError("Clock enable disable_on_write does not accept mask, match, or order");
    cfg.mode = ClockEnable::DisableOnWrite{};
  } else if (mode == "enable_when") {
    const u64 mask = has("mask") ? as_u64(node["mask"]) : ~u64{0};
    const auto order = parse_order(node);
    if (has("match"))
      cfg.mode = ClockEnable::EnableWhenEqual{.mask = mask, .match = as_u64(node["match"]), .order = order};
    else cfg.mode = ClockEnable::EnableWhenAny{.mask = mask, .order = order};
  } else throw ParsingError("Clock enable mode must be one of: disable_on_write, enable_when");
  return cfg;
}

// Visitor to serialize the fields specific to each ClockEnable::Mode.
struct SerializeMode {
  nlohmann::json &node;
  void operator()(const ClockEnable::DisableOnWrite &) const { node["mode"] = "disable_on_write"; }
  void operator()(const ClockEnable::EnableWhenAny &m) const { enable_when(m.mask, m.order); }
  void operator()(const ClockEnable::EnableWhenEqual &m) const {
    enable_when(m.mask, m.order);
    node["match"] = m.match;
  }

private:
  void enable_when(u64 mask, bits::Order order) const {
    node["mode"] = "enable_when";
    if (mask != ~u64{0}) node["mask"] = mask;
    if (order != bits::Order::LittleEndian) node["order"] = "big";
  }
};

void serialize_enable(nlohmann::json &obj, const std::optional<ClockEnable::Configuration> &enable) {
  if (!enable) return;
  nlohmann::json node;
  node["source"] = enable->source;
  if (enable->span) {
    node["offset"] = enable->span->lower();
    node["width"] = pepp::core::size_inclusive(*enable->span);
  }
  std::visit(SerializeMode{node}, enable->mode);
  obj["enable"] = std::move(node);
}

Device *create_ideal_clock(const nlohmann::json &self, System *sys, Device *par) {
  IdealClock::Configuration cfg;
  try {
    parse_standard_fields(self, cfg);
    if (cfg.basename.empty()) throw ParsingError("IdealClock must have a basename");
    if (!self.contains("period") || self["period"].is_null()) throw ParsingError("IdealClock must have a period");
    cfg.period = as_u64(self["period"]);
    cfg.enable = parse_enable(self);
  } catch (const nlohmann::json::type_error &e) {
    throw ParsingError("Failed to parse IdealClock: " + std::string(e.what()));
  }
  if (cfg.period == 0) throw ParsingError("IdealClock must have a non-zero period");
  return sys->make_device<IdealClock>(par, cfg);
}

void prefill_ideal_clock(nlohmann::json &obj) {
  obj["compatible"] = IdealClock::compatible;
  obj["basename"];
  obj["period"];
  obj["enable"] = nullptr;
}

void serialize_ideal_clock(nlohmann::json &obj, const System *sys, const Device *self) {
  auto casted = dynamic_cast<const IdealClock *>(self);
  if (!casted) throw std::logic_error("serialize_ideal_clock called on non-IdealClock device");
  obj["compatible"] = IdealClock::compatible;
  obj["basename"] = casted->config().basename;
  obj["period"] = casted->casted_config().period;
  serialize_enable(obj, casted->casted_config().enable);
}

Device *create_scaled_clock(const nlohmann::json &self, System *sys, Device *par) {
  ScaledClock::Configuration cfg;
  try {
    parse_standard_fields(self, cfg);
    if (cfg.basename.empty()) throw ParsingError("ScaledClock must have a basename");
    if (!self.contains("parent") || self["parent"].is_null()) throw ParsingError("ScaledClock must have a parent");
    cfg.parent_name = self["parent"].get<std::string>();
    if (!self.contains("period_scale") || self["period_scale"].is_null())
      throw ParsingError("ScaledClock must have a period_scale");
    cfg.period_scale = self["period_scale"].get<float>();
    // Left as NaN so the ctor can copy period_scale into it.
    if (self.contains("jitter_scale") && !self["jitter_scale"].is_null())
      cfg.jitter_scale = self["jitter_scale"].get<float>();
    cfg.enable = parse_enable(self);
  } catch (const nlohmann::json::type_error &e) {
    throw ParsingError("Failed to parse ScaledClock: " + std::string(e.what()));
  }
  return sys->make_device<ScaledClock>(par, cfg);
}

void prefill_scaled_clock(nlohmann::json &obj) {
  obj["compatible"] = ScaledClock::compatible;
  obj["basename"];
  obj["parent"];
  obj["period_scale"];
  obj["jitter_scale"] = nullptr;
  obj["enable"] = nullptr;
}

void serialize_scaled_clock(nlohmann::json &obj, const System *sys, const Device *self) {
  auto casted = dynamic_cast<const ScaledClock *>(self);
  if (!casted) throw std::logic_error("serialize_scaled_clock called on non-ScaledClock device");
  obj["compatible"] = ScaledClock::compatible;
  obj["basename"] = casted->config().basename;
  obj["parent"] = casted->casted_config().parent_name;
  obj["period_scale"] = casted->casted_config().period_scale;
  // TODO: does the qNan actually serialize as expected?
  obj["jitter_scale"] = casted->casted_config().jitter_scale;
  serialize_enable(obj, casted->casted_config().enable);
}

Device *create_mux_clock(const nlohmann::json &self, System *sys, Device *par) {
  MuxClock::Configuration cfg;
  try {
    parse_standard_fields(self, cfg);
    if (cfg.basename.empty()) throw ParsingError("MuxClock must have a basename");
    if (!self.contains("choices") || !self["choices"].is_array())
      throw ParsingError("MuxClock must have an array of choices");
    for (const auto &choice : self["choices"]) cfg.names.push_back(choice.get<std::string>());
    if (self.contains("selected") && !self["selected"].is_null()) cfg.selected = as_u16(self["selected"]);
    cfg.enable = parse_enable(self);
  } catch (const nlohmann::json::type_error &e) {
    throw ParsingError("Failed to parse MuxClock: " + std::string(e.what()));
  }
  if (cfg.names.empty()) throw ParsingError("MuxClock must have at least one choice");
  if (cfg.selected >= cfg.names.size()) throw ParsingError("MuxClock selected is out of range");
  return sys->make_device<MuxClock>(par, cfg);
}

void prefill_mux_clock(nlohmann::json &obj) {
  obj["compatible"] = MuxClock::compatible;
  obj["basename"];
  obj["choices"] = nlohmann::json::array();
  obj["selected"] = 0;
  obj["enable"] = nullptr;
}

void serialize_mux_clock(nlohmann::json &obj, const System *sys, const Device *self) {
  auto casted = dynamic_cast<const MuxClock *>(self);
  if (!casted) throw std::logic_error("serialize_mux_clock called on non-MuxClock device");
  obj["compatible"] = MuxClock::compatible;
  obj["basename"] = casted->config().basename;
  obj["choices"] = casted->casted_config().names;
  obj["selected"] = casted->casted_config().selected;
  serialize_enable(obj, casted->casted_config().enable);
}
} // namespace

Device::Type pepp::ClockNode::type() const {
  using namespace bits;
  using T = Device::Type;
  return T::ClockSource | T::EventSource | T::EventSink;
}

void pepp::ClockNode::initialize(System *sys) {
  if (auto *sink = sys->capability<EventSink>(); sink != nullptr) subscribe(sink);
  if (!_enable) return;
  const auto &name = _enable->source;
  auto *dev = sys->find_relative(name, config().fullname);
  if (!dev) throw std::runtime_error("Clock enable: could not find source " + name);
  auto *target = dev->capability<Target>();
  auto *events = dev->capability<EventSource>();
  if (!target || !events) throw std::runtime_error("Clock enable: " + name + " must be a Target and an EventSource");

  const auto span = target->span();
  _watched = _enable->span.value_or(span);
  if (!pepp::core::contains(span, _watched))
    throw std::runtime_error("Clock enable: watched range is outside of " + name);
  if (!std::holds_alternative<ClockEnable::DisableOnWrite>(_enable->mode) && pepp::core::size_inclusive(_watched) > 8)
    throw std::runtime_error("Clock enable: enable_when may watch at most 8 bytes of " + name);
  _source = target;
  events->subscribe(this);
}

u64 pepp::ClockNode::read_watched(bits::Order order) const {
  std::array<u8, 8> bytes{};
  const auto used = bits::span<u8>{bytes}.first(pepp::core::size_inclusive(_watched));
  _source->read(_watched.lower(), used, Operation{Operation::Type::BufferInternal, Operation::Kind::data});
  return bits::memcpy_endian<u64>(bits::span<const u8>{used}, order);
}

struct pepp::ClockNode::SelfEnabled {
  const ClockNode &node;
  bool operator()(const ClockEnable::DisableOnWrite &) const { return !node._latched_off; }
  bool operator()(const ClockEnable::EnableWhenAny &m) const { return (node.read_watched(m.order) & m.mask) != 0; }
  bool operator()(const ClockEnable::EnableWhenEqual &m) const {
    return (node.read_watched(m.order) & m.mask) == m.match;
  }
};

bool pepp::ClockNode::self_enabled() const {
  if (!_enable || !_source) return true;
  return std::visit(SelfEnabled{*this}, _enable->mode);
}

void pepp::ClockNode::on_event(Device::ID, const Event &event) {
  auto *written = dynamic_cast<const MemoryWritten *>(&event);
  // Ignore other events and ignore writes to the wrong locations/targets
  if (!written || !_enable || written->target != _source) return;
  else if (!pepp::core::intersects(written->span, _watched)) return;
  if (std::holds_alternative<ClockEnable::DisableOnWrite>(_enable->mode)) {
    if (_latched_off) return;
    _latched_off = true;
  }
  // Inform the simulator to recompure the schedule for this clock, even if there may have been no change.
  // While potentially expensive, this should be incredibly rare.
  schedule_changed();
}

PulseSchedule pepp::ClockNode::apply_enable(PulseSchedule sched) const {
  sched.enabled = sched.enabled && self_enabled();
  return sched;
}

pepp::ScaledClock::ScaledClock(Configuration config) : ClockNode(config.enable), _config(config) {
  if (_config.jitter_scale != _config.jitter_scale) _config.jitter_scale = _config.period_scale;
}

void pepp::ScaledClock::initialize(System *sys) {
  auto dev = sys->find_relative(_config.parent_name, _config.fullname);
  if (!dev) throw std::runtime_error("ScaledClockNode: could not find parent clock " + _config.parent_name);
  auto clk = dev->capability<ClockSource>();
  if (!clk) throw std::runtime_error("ScaledClockNode: device " + _config.parent_name + " is not a clock source");
  _parent = clk;
  ClockNode::initialize(sys);
}

PulseSchedule pepp::ScaledClock::schedule() const {
  if (!_parent) throw std::runtime_error("ScaledClockNode: parent clock not set");
  const auto par = _parent->schedule();
  return apply_enable(PulseSchedule{.period = static_cast<u64>(par.period * _config.period_scale),
                             .jitter = static_cast<u64>(par.jitter * _config.jitter_scale),
                             .seed = par.seed,
                             .enabled = par.enabled});
}

std::unique_ptr<DeviceSerializer> pepp::ScaledClock::serializer() const { return make_serializer(); }

std::unique_ptr<DeviceSerializer> pepp::ScaledClock::make_serializer() {
  DeviceSerializer s{.parser = create_scaled_clock,
                     .prefill = prefill_scaled_clock,
                     .serialize = serialize_scaled_clock,
                     .compatible = ScaledClock::compatible};
  return std::make_unique<DeviceSerializer>(std::move(s));
}

pepp::MuxClock::MuxClock(Configuration config) : ClockNode(config.enable), _index(0), _config(config) {}

void pepp::MuxClock::initialize(System *sys) {
  _choices.clear();
  for (const auto &name : _config.names) {
    auto dev = sys->find_relative(name, _config.fullname);
    if (!dev) throw std::runtime_error("MuxClockNode: could not find clock " + name);
    auto clk = dev->capability<ClockSource>();
    if (!clk) throw std::runtime_error("MuxClockNode: device " + name + " is not a clock source");
    _choices.push_back(clk);
  }
  select_clock(_config.selected);
  ClockNode::initialize(sys);
}

void pepp::MuxClock::select_clock(u16 index) {
  if (index >= _choices.size()) throw std::runtime_error("MuxClockNode: index out of range");
  else if (index == _index) return; // No change
  _index = index;
  schedule_changed();
}

PulseSchedule pepp::MuxClock::schedule() const {
  if (_index >= _choices.size()) throw std::runtime_error("MuxClockNode: index out of range");
  return apply_enable(_choices[_index]->schedule());
}

std::unique_ptr<DeviceSerializer> pepp::MuxClock::serializer() const { return make_serializer(); }

std::unique_ptr<DeviceSerializer> pepp::MuxClock::make_serializer() {
  DeviceSerializer s{.parser = create_mux_clock,
                     .prefill = prefill_mux_clock,
                     .serialize = serialize_mux_clock,
                     .compatible = MuxClock::compatible};
  return std::make_unique<DeviceSerializer>(std::move(s));
}

const ClockSource *pepp::MuxClock::selected_clock() const {
  if (_index > _choices.size()) return nullptr;
  else return _choices[_index];
}

std::unique_ptr<DeviceSerializer> pepp::IdealClock::serializer() const { return make_serializer(); }

std::unique_ptr<DeviceSerializer> pepp::IdealClock::make_serializer() {
  DeviceSerializer s{.parser = create_ideal_clock,
                     .prefill = prefill_ideal_clock,
                     .serialize = serialize_ideal_clock,
                     .compatible = IdealClock::compatible};
  return std::make_unique<DeviceSerializer>(std::move(s));
}
