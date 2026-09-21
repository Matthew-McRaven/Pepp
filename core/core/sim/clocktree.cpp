#include "./clocktree.hpp"
#include <nlohmann/json.hpp>
#include "core/sim/systemparser.hpp"
#include "system.hpp"

namespace {
using namespace pepp;

Device *create_ideal_clock(const nlohmann::json &self, System *sys, Device *par) {
  IdealClock::Configuration cfg;
  try {
    parse_standard_fields(self, cfg);
    if (cfg.basename.empty()) throw ParsingError("IdealClock must have a basename");
    if (!self.contains("period") || self["period"].is_null()) throw ParsingError("IdealClock must have a period");
    cfg.period = as_u64(self["period"]);
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
}

void serialize_ideal_clock(nlohmann::json &obj, const System *sys, const Device *self) {
  auto casted = dynamic_cast<const IdealClock *>(self);
  if (!casted) throw std::logic_error("serialize_ideal_clock called on non-IdealClock device");
  obj["compatible"] = IdealClock::compatible;
  obj["basename"] = casted->config().basename;
  obj["period"] = casted->casted_config().period;
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
}

void serialize_mux_clock(nlohmann::json &obj, const System *sys, const Device *self) {
  auto casted = dynamic_cast<const MuxClock *>(self);
  if (!casted) throw std::logic_error("serialize_mux_clock called on non-MuxClock device");
  obj["compatible"] = MuxClock::compatible;
  obj["basename"] = casted->config().basename;
  obj["choices"] = casted->casted_config().names;
  obj["selected"] = casted->casted_config().selected;
}
} // namespace

pepp::ScaledClock::ScaledClock(Configuration config) : Device(), ClockSource(), _config(config) {
  if (_config.jitter_scale != _config.jitter_scale) _config.jitter_scale = _config.period_scale;
}

void pepp::ScaledClock::initialize(System *sys) {
  auto dev = sys->find_relative(_config.parent_name, _config.fullname);
  if (!dev) throw std::runtime_error("ScaledClockNode: could not find parent clock " + _config.parent_name);
  auto clk = dev->capability<ClockSource>();
  if (!clk) throw std::runtime_error("ScaledClockNode: device " + _config.parent_name + " is not a clock source");
  _parent = clk;
}

PulseSchedule pepp::ScaledClock::schedule() const {
  if (!_parent) throw std::runtime_error("ScaledClockNode: parent clock not set");
  const auto par = _parent->schedule();
  return PulseSchedule{.period = static_cast<u64>(par.period * _config.period_scale),
                       .jitter = static_cast<u64>(par.jitter * _config.jitter_scale),
                       .seed = par.seed};
}

std::unique_ptr<DeviceSerializer> pepp::ScaledClock::serializer() const { return make_serializer(); }

std::unique_ptr<DeviceSerializer> pepp::ScaledClock::make_serializer() {
  DeviceSerializer s{.parser = create_scaled_clock,
                     .prefill = prefill_scaled_clock,
                     .serialize = serialize_scaled_clock,
                     .compatible = ScaledClock::compatible};
  return std::make_unique<DeviceSerializer>(std::move(s));
}

pepp::MuxClock::MuxClock(Configuration config) : Device(), ClockSource(), _index(0), _config(config) {}

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
}

void pepp::MuxClock::select_clock(u16 index) {
  if (index >= _choices.size()) throw std::runtime_error("MuxClockNode: index out of range");
  else if (index == _index) return; // No change
  _index = index;
}

PulseSchedule pepp::MuxClock::schedule() const {
  if (_index >= _choices.size()) throw std::runtime_error("MuxClockNode: index out of range");
  return _choices[_index]->schedule();
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
