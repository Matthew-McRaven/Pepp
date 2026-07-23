#include "pep_isa.hpp"
#include <nlohmann/json.hpp>
#include "core/arch/pep/isa/pep10.hpp"
#include "core/ds/string_compare.hpp"
#include "core/sim/cores/cpu/pep_isa_instructions.hpp"
#include "core/sim/memory/ram/dense.hpp"
#include "core/sim/system.hpp"
#include "core/sim/systemparser.hpp"

namespace {
static const bool swap = bits::hostOrder() != bits::Order::BigEndian;
static const Operation rw_d{.type = Operation::Type::Standard, .kind = Operation::Kind::data};

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
  using enum isa::SharedOpBehavior;
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

void PepISA3CPU::clock_tick(PulseSchedule::PulseIndex idx, u64 tick) {
  // Fetch & increment pc
  auto pc = read_register(isa::Pep10::Register::PC);
  u8 is = _target->read<u8, false>(pc, rw_d).second;
  pc += 1;
  write_register(isa::Pep10::Register::PC, pc);
  write_register(isa::Pep10::Register::IS, is);
  handle(_opcodes[is]);
  // TODO: handle breakpoints, debug info, etc
}

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

void PepISA3CPU::increment_call_depth() {
  // TODO:
}

void PepISA3CPU::decrement_call_depth() {
  // TODO:
}

u8 PepISA3CPU::read_packed_csr() {
  const auto size = size_inclusive(_csrs->span());
  u8 ret = 0;
  for (u8 i = 0; i < size; ++i) {
    auto bit = ((Target *)_csrs)->read<u8, false>(i, rw_d).second;
    ret |= (bit ? 1 : 0) << i;
  }
  return ret;
}

void PepISA3CPU::write_packed_csr(u8 value) {
  const auto size = size_inclusive(_csrs->span());
  for (u8 i = 0; i < size; ++i) {
    auto bit = (value >> i) & 1;
    ((Target *)_csrs)->write<u8, false>(i, bit, rw_d);
  }
}

void PepISA3CPU::handle(Op opcode) {
  using R = isa::Pep10::Register;
  using BC = BranchCondition;
  using enum isa::SharedOpBehavior;
  // Monadic
  switch (opcode.behavior) {
  case UNIMPL: return unimpl_handler(this);
  case RET: return handle_ret(this);
  case SRET: return handle_sret(this);
  case MOVFLGA: return handle_movflga(this);
  case MOVAFLG: return handle_movaflg(this);
  case MOVSPA: return handle_movspa(this);
  case MOVASP: return handle_movasp(this);
  case HW_NOP: return handle_nop(this);
  case NEG: return handle_negr(this, (R)opcode.target);
  case ASL: return handle_aslr(this, (R)opcode.target);
  case ASR: return handle_asrr(this, (R)opcode.target);
  case NOT: return handle_notr(this, (R)opcode.target);
  case ROL: return handle_rolr(this, (R)opcode.target);
  case ROR: return handle_rorr(this, (R)opcode.target);
  case SCALL: throw std::logic_error("Unimplemented instruction: SCALL");
  case TRAP_CALL: throw std::logic_error("Unimplemented instruction: TRAP_CALL");
  default: break;
  }

  // Dyadic
  u16 op_addr = decode_op_addr(this, opcode.addr);
  switch (opcode.behavior) {
  case BR: return handle_branch(this, opcode, BC::UNCONDITIONAL, op_addr);
  case BRLE: return handle_branch(this, opcode, BC::LE, op_addr);
  case BRLT: return handle_branch(this, opcode, BC::LT, op_addr);
  case BREQ: return handle_branch(this, opcode, BC::EQ, op_addr);
  case BRNE: return handle_branch(this, opcode, BC::NE, op_addr);
  case BRGE: return handle_branch(this, opcode, BC::GE, op_addr);
  case BRGT: return handle_branch(this, opcode, BC::GT, op_addr);
  case BRV: return handle_branch(this, opcode, BC::V, op_addr);
  case BRC: return handle_branch(this, opcode, BC::C, op_addr);
  case CALL: return handle_call(this, opcode, op_addr);
  case ADDSP: return handle_addsp(this, opcode, op_addr);
  case SUBSP: return handle_subsp(this, opcode, op_addr);
  case ADD: return handle_addr(this, opcode, op_addr);
  case SUB: return handle_subr(this, opcode, op_addr);
  case AND: return handle_bitopr(this, opcode, Bitop::AND, op_addr);
  case OR: return handle_bitopr(this, opcode, Bitop::OR, op_addr);
  case XOR: return handle_bitopr(this, opcode, Bitop::XOR, op_addr);
  case CPW: return handle_cpwr(this, opcode, op_addr);
  case CPB: return handle_cpbr(this, opcode, op_addr);
  case LDW: return handle_ldwr(this, opcode, op_addr);
  case LDB: return handle_ldbr(this, opcode, op_addr);
  case STW: return handle_stwr(this, opcode, op_addr);
  case STB: return handle_stbr(this, opcode, op_addr);
  default: throw std::logic_error("Unknown opcode behavior");
  }
}
Target *PepISA3CPU::target() { return _target; }
