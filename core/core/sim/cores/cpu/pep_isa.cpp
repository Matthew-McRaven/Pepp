#include "pep_isa.hpp"
#include <nlohmann/json.hpp>
#include "core/ds/string_compare.hpp"
#include "core/sim/memory/ram/dense.hpp"
#include "core/sim/system.hpp"
#include "core/sim/systemparser.hpp"

namespace {
static const std::unordered_map<std::string, PepISA3CPU::ISA, pepp::bts::ci_hash, pepp::bts::ci_eq> map_str_to_isa = {
    {"pep8", PepISA3CPU::ISA::Pep8}, {"pep9", PepISA3CPU::ISA::Pep9}, {"pep10", PepISA3CPU::ISA::Pep10}};
std::optional<PepISA3CPU::ISA> string_to_isa(std::string_view str) {
  auto it = map_str_to_isa.find(std::string(str));
  if (it != map_str_to_isa.end()) return it->second;
  return std::nullopt;
}
std::string isa_to_string(PepISA3CPU::ISA isa) {
  for (const auto &[key, value] : map_str_to_isa)
    if (value == isa) return key;
  throw std::runtime_error("Unknown ISA enum value");
}

Device *create_pepisacpu(const nlohmann::json &self, System *sys, Device *par) {
  PepISA3CPU::Configuration cfg;
  try {
    parse_standard_fields(self, cfg);
    if (cfg.basename.empty()) throw ParsingError("PepISA3CPU must have a basename");
    if (!self.contains("target") || self["target"].is_null()) throw ParsingError("PepISA3CPU must have a target");
    cfg.target = self["target"].get<std::string>();
    if (self.contains("isa") && !self["isa"].is_null()) {
      auto isa_str = self["isa"].get<std::string>();
      auto isa_opt = string_to_isa(isa_str);
      if (!isa_opt) throw ParsingError("PepISA3CPU: unknown ISA " + isa_str);
      cfg.isa = *isa_opt;
    }
  } catch (const nlohmann::json::type_error &e) {
    throw ParsingError("Failed to parse PepISA3CPU: " + std::string(e.what()));
  }
  return sys->make_device<PepISA3CPU>(par, cfg, sys);
}

void prefill_pepisacpu(nlohmann::json &obj) {
  obj["compatible"] = PepISA3CPU::compatible;
  obj["basename"];
  obj["target"];
  obj["isa"] = isa_to_string(PepISA3CPU::ISA::Pep10);
}

void serialize_pepisacpu(nlohmann::json &obj, const System *sys, const Device *self) {
  auto casted = dynamic_cast<const PepISA3CPU *>(self);
  if (!casted) throw std::logic_error("serialize_pepisacpu called on non-PepISA3CPU device");
  obj["compatible"] = PepISA3CPU::compatible;
  obj["basename"] = casted->config().basename;
  obj["target"] = casted->casted_config().target;
  obj["isa"] = isa_to_string(casted->casted_config().isa);
}
} // namespace

PepISA3CPU::PepISA3CPU(Configuration cfg, System *sys) : _config(cfg) {}

const Target *PepISA3CPU::target() const { return _target; }

void PepISA3CPU::initialize(System *sys) {
  auto dev = sys->find_relative(_config.target, _config.fullname);
  if (!dev) throw std::runtime_error("PepISA3CPU: could not find target device " + _config.target);
  _target = dev->capability<Target>();
  if (!_target) throw std::runtime_error("PepISA3CPU: device " + _config.target + " is not a memory target");
  {
    Dense::Configuration cfg;
    cfg.basename = "regs";
    cfg.fill = 0;
    cfg.span = {0, 31 * sizeof(u16)};
    cfg.skip_serialize = true;
    _regbank = sys->make_device<Dense>(this, cfg);
    _regbank->initialize(sys);
  }
  {
    Dense::Configuration cfg;
    cfg.basename = "csrs";
    cfg.fill = 0;
    // N, Z, V, C
    cfg.span = {0, 3};
    cfg.skip_serialize = true;
    _csrs = sys->make_device<Dense>(this, cfg);
    _csrs->initialize(sys);
  }
}

const Device::Configuration &PepISA3CPU::config() const { return _config; }

const PepISA3CPU::Configuration &PepISA3CPU::casted_config() const { return _config; }

const Device::ID PepISA3CPU::id() const { return _config.id; }

Device::Type PepISA3CPU::type() const {
  using namespace bits;
  using T = Device::Type;
  return T::ClockSink | T::Traceable | T::MemoryInitiator;
}

std::unique_ptr<DeviceSerializer> PepISA3CPU::serializer() const { return make_serializer(); }

std::unique_ptr<DeviceSerializer> PepISA3CPU::make_serializer() {
  DeviceSerializer s{.parser = create_pepisacpu,
                     .prefill = prefill_pepisacpu,
                     .serialize = serialize_pepisacpu,
                     .compatible = PepISA3CPU::compatible};
  return std::make_unique<DeviceSerializer>(std::move(s));
}

void PepISA3CPU::clock_tick(PulseSchedule::PulseIndex idx, u64 tick) { (void)idx, (void)tick; }

void PepISA3CPU::set_clock_source(const ClockSource *src) { _clk = src; }

const ClockSource *PepISA3CPU::clock_source() const { return _clk; }

void PepISA3CPU::set_buffer(Buffer *tb) {
  _tb = tb;
  _regbank->set_buffer(tb);
  _csrs->set_buffer(tb);
}

const Buffer *PepISA3CPU::buffer() const { return _tb; }

bool PepISA3CPU::can_generate_traces() const { return true; }

void PepISA3CPU::trace(bool enabled) {
  if (_tb) {
    _tb->trace(id(), enabled);
    _regbank->trace(enabled);
    _csrs->trace(enabled);
  }
}

bool PepISA3CPU::traced() const { return _tb ? _tb->traced(id()) : false; }
