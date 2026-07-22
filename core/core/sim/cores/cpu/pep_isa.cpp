#include "pep_isa.hpp"
#include <nlohmann/json.hpp>
#include "core/arch/pep/isa/pep10.hpp"
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

static const bool swap = bits::hostOrder() != bits::Order::BigEndian;

using Opcode = isa::detail::Opcode;
void unimpl_handler(PepISA3CPU *self, Opcode op) { throw std::logic_error("Unimplemented instruction encountered"); }

void handle_ret(PepISA3CPU *self, Opcode op) {
  // self->decrement_call_depth();
  // u16 sp = self->read_reg(sp)
  u16 sp;
  self->target()->read<u16, swap>(sp, {});
  // self->write_reg(0);
}

void handle_movflga(PepISA3CPU *self, Opcode op) {
  // read NZVC
  // write to A.
}

PepISA3CPU::PepISA3CPU(Configuration cfg, System *sys) : _config(cfg) {
  auto make_regs = [](System *sys, Device::ID parent) {
    auto device = sys->find_by_id(parent);
    auto self = dynamic_cast<PepISA3CPU *>(device);
    Dense::Configuration cfg;
    cfg.basename = "regs";
    cfg.fill = 0;
    cfg.span = {0, 31 * sizeof(u16)};
    cfg.skip_serialize = true;
    self->_regbank = sys->make_device<Dense>(parent, cfg);
  };
  sys->make_deferred(DeferredDevice{.parent = _config.id, .ctor = make_regs});
  auto make_csrs = [](System *sys, Device::ID parent) {
    auto device = sys->find_by_id(parent);
    auto self = dynamic_cast<PepISA3CPU *>(device);
    Dense::Configuration cfg;
    cfg.basename = "csrs";
    cfg.fill = 0;
    // N, Z, V, C
    cfg.span = {0, 3};
    cfg.skip_serialize = true;
    self->_csrs = sys->make_device<Dense>(parent, cfg);
  };
  sys->make_deferred(DeferredDevice{.parent = _config.id, .ctor = make_csrs});
}

const Target *PepISA3CPU::target() const { return _target; }

void PepISA3CPU::initialize(System *sys) {
  using enum isa::detail::OpBehavior;
  auto dev = sys->find_relative(_config.target, _config.fullname);
  if (!dev) throw std::runtime_error("PepISA3CPU: could not find target device " + _config.target);
  _target = dev->capability<Target>();
  if (!_target) throw std::runtime_error("PepISA3CPU: device " + _config.target + " is not a memory target");
  switch (_config.isa) {
  case ISA::Pep8: throw std::logic_error("PepISA3CPU: ISA " + isa_to_string(_config.isa) + " not implemented");
  case ISA::Pep9: throw std::logic_error("PepISA3CPU: ISA " + isa_to_string(_config.isa) + " not implemented");
  case ISA::Pep10: _opcodes = isa::Pep10::opcode_plane; break;
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

void PepISA3CPU::handle(isa::detail::Opcode opcode) {
  switch (opcode.behavior) {
  case isa::detail::OpBehavior::UNIMPL: unimpl_handler(this, opcode); break;
  case isa::detail::OpBehavior::RET: break;
  case isa::detail::OpBehavior::SRET: break;
  case isa::detail::OpBehavior::MOVFLGA: break;
  case isa::detail::OpBehavior::MOVAFLG: break;
  case isa::detail::OpBehavior::MOVSPA: break;
  case isa::detail::OpBehavior::MOVASP: break;
  case isa::detail::OpBehavior::NEGA: break;
  case isa::detail::OpBehavior::NEGX: break;
  case isa::detail::OpBehavior::ASLA: break;
  case isa::detail::OpBehavior::ASLX: break;
  case isa::detail::OpBehavior::ASRA: break;
  case isa::detail::OpBehavior::ASRX: break;
  case isa::detail::OpBehavior::NOTA: break;
  case isa::detail::OpBehavior::NOTX: break;
  case isa::detail::OpBehavior::ROLA: break;
  case isa::detail::OpBehavior::ROLX: break;
  case isa::detail::OpBehavior::RORA: break;
  case isa::detail::OpBehavior::RORX: break;
  case isa::detail::OpBehavior::BR: break;
  case isa::detail::OpBehavior::BRLE: break;
  case isa::detail::OpBehavior::BRLT: break;
  case isa::detail::OpBehavior::BREQ: break;
  case isa::detail::OpBehavior::BRNE: break;
  case isa::detail::OpBehavior::BRGE: break;
  case isa::detail::OpBehavior::BRGT: break;
  case isa::detail::OpBehavior::BRV: break;
  case isa::detail::OpBehavior::BRC: break;
  case isa::detail::OpBehavior::CALL: break;
  case isa::detail::OpBehavior::SCALL: break;
  case isa::detail::OpBehavior::TRAP_CALL: break;
  case isa::detail::OpBehavior::ADDSP: break;
  case isa::detail::OpBehavior::SUBSP: break;
  case isa::detail::OpBehavior::ADDA: break;
  case isa::detail::OpBehavior::ADDX: break;
  case isa::detail::OpBehavior::SUBA: break;
  case isa::detail::OpBehavior::SUBX: break;
  case isa::detail::OpBehavior::ANDA: break;
  case isa::detail::OpBehavior::ANDX: break;
  case isa::detail::OpBehavior::ORA: break;
  case isa::detail::OpBehavior::ORX: break;
  case isa::detail::OpBehavior::XORA: break;
  case isa::detail::OpBehavior::XORX: break;
  case isa::detail::OpBehavior::CPWA: break;
  case isa::detail::OpBehavior::CPWX: break;
  case isa::detail::OpBehavior::CPBA: break;
  case isa::detail::OpBehavior::CPBX: break;
  case isa::detail::OpBehavior::LDWA: break;
  case isa::detail::OpBehavior::LDWX: break;
  case isa::detail::OpBehavior::LDBA: break;
  case isa::detail::OpBehavior::LDBX: break;
  case isa::detail::OpBehavior::STWA: break;
  case isa::detail::OpBehavior::STWX: break;
  case isa::detail::OpBehavior::STBA: break;
  case isa::detail::OpBehavior::STBX: break;
  default: throw std::logic_error("Unknown opcode behavior");
  }
}
