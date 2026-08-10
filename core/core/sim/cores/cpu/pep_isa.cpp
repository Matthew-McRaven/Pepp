#include "pep_isa.hpp"
#include <array>
#include <nlohmann/json.hpp>
#include "core/arch/pep/isa/pep10.hpp"
#include "core/arch/pep/isa/pep9.hpp"
#include "core/ds/string_compare.hpp"
#include "core/sim/cores/cpu/pep_isa_instructions.hpp"
#include "core/sim/debugger/register_scanner.hpp"
#include "core/sim/memory/ram/dense.hpp"
#include "core/sim/system.hpp"
#include "core/sim/systemparser.hpp"

namespace {
static const bool swap = bits::hostOrder() != bits::Order::BigEndian;

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
  case ISA::Pep9: _opcodes = isa::Pep9::opcode_plane; break;
  case ISA::Pep10: _opcodes = isa::Pep10::opcode_plane; break;
  }

  auto scan = sys->register_scan();
  // Scanned register
  using SR = RegisterScan::Register;
  static const auto BE = bits::Order::BigEndian;
  static const auto LE = bits::Order::LittleEndian;
  static const auto RW = RegisterScan::Register::ReadWrite;
  // Core registers
  using R = isa::Pep10::Register;
  const auto rid = _regbank->id();
  static const auto r2i = [](const R &r) -> u16 { return static_cast<u16>(r) * 2; };
  scan->expose(SR{.order = BE, .byte_width = 2, .access = RW, .target = rid, .offset = r2i(R::A), .name = "A"});
  scan->expose(SR{.order = BE, .byte_width = 2, .access = RW, .target = rid, .offset = r2i(R::X), .name = "X"});
  scan->expose(SR{.order = BE, .byte_width = 2, .access = RW, .target = rid, .offset = r2i(R::PC), .name = "PC"});
  scan->expose(SR{.order = BE, .byte_width = 2, .access = RW, .target = rid, .offset = r2i(R::SP), .name = "SP"});
  scan->expose(SR{.order = BE, .byte_width = 1, .access = RW, .target = rid, .offset = r2i(R::IS) + 1, .name = "IS"});
  scan->expose(SR{.order = BE, .byte_width = 2, .access = RW, .target = rid, .offset = r2i(R::OS), .name = "OS"});
  // CSRs / Flags
  using C = isa::Pep10::CSR;
  const auto cid = _csrs->id();
  using F = SR::Field;
  // Should really be 4 separate fields, but I want to test that my fields work as expected.
  // When moving to 4 fields, no need for bit offsets.
  // Bit 31 is MSB, 0 is LSB. Considering these are 4 consecutive bytes, the offsets make sense.
  auto n = F{.access = RW, .bit_offset = 24, .bit_width = 1, .name = "N"};
  auto z = F{.access = RW, .bit_offset = 16, .bit_width = 1, .name = "Z"};
  auto v = F{.access = RW, .bit_offset = 8, .bit_width = 1, .name = "V"};
  auto c = F{.access = RW, .bit_offset = 0, .bit_width = 1, .name = "C"};
  scan->expose(SR{
      .order = BE, .byte_width = 4, .access = RW, .target = cid, .offset = 0, .name = "NZVC", .fields = {n, z, v, c}});
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
  // Create a single record for the entire instruction
  trace::Recorder::Instruction record(_trace);
  // TODO: when function signature changes, use that tick offset instead of this placeholder.
  record.tick(1);

  // Take PC out of the register bank for the duration of this instruction. Everything below moves it through _pc,
  // and the single store after handle() is the only version the trace ever sees. See read_pc().
  _pc = read_register_uncached(isa::Pep10::Register::PC);
  // TODO: Should probably be an instruction access?
  u8 is = _target->read<u8, false>(_pc, op_data()).second;
  _pc += 1;
  write_register(isa::Pep10::Register::IS, is);
  handle(_opcodes[is]);
  // Defer PC writeback until end of instruction to avoid ~3 updates on a BR (1 for to fetch IS, 1 to fetch OS, 1 for
  // the branch).
  write_register_uncached(isa::Pep10::Register::PC, _pc);
  // TODO: handle breakpoints, debug info, etc
  record.commit();
}

void PepISA3CPU::set_clock_source(const ClockSource *src) { _clk = src; }

const ClockSource *PepISA3CPU::clock_source() const { return _clk; }

void PepISA3CPU::set_recorder(const trace::Recorder &recorder) { _trace = recorder; }

bool PepISA3CPU::can_generate_traces() const { return true; }

bool PepISA3CPU::traced() const { return _trace.traced(); }

void PepISA3CPU::trace(bool enabled) {
  // The CPU is not itself a Target and it holds no state to record, so delegate to the child devices.
  _trace.set_traced(enabled);
  if (_regbank) _regbank->trace(enabled);
  if (_csrs) _csrs->trace(enabled);
}

void PepISA3CPU::increment_call_depth() {
  // TODO:
}

void PepISA3CPU::decrement_call_depth() {
  // TODO:
}

// The CSR bank stores one flag per byte, in CSR enum order: N at 0, Z at 1, V at 2, C at 3. Both of these move all
// four in a single access rather than one per flag. Perform as a single batched read/write to reduce the # of traces
// emitted for this operation.
u8 PepISA3CPU::read_packed_csr() {
  std::array<u8, 4> nzvc{};
  ((Target *)_csrs)->read(0, {nzvc.data(), nzvc.size()}, op_data());
  return static_cast<u8>((nzvc[0] ? 1 << 3 : 0) | (nzvc[1] ? 1 << 2 : 0) | (nzvc[2] ? 1 << 1 : 0) |
                         (nzvc[3] ? 1 << 0 : 0));
}

void PepISA3CPU::write_packed_csr(u8 value) {
  const std::array<u8, 4> nzvc{
      static_cast<u8>((value >> 3) & 1),
      static_cast<u8>((value >> 2) & 1),
      static_cast<u8>((value >> 1) & 1),
      static_cast<u8>((value >> 0) & 1),
  };
  ((Target *)_csrs)->write(0, {nzvc.data(), nzvc.size()}, op_data());
}

void PepISA3CPU::handle(Op opcode) {
  using R = isa::Pep10::Register;
  using BC = BranchCondition;
  using enum isa::SharedOpBehavior;
  // Monadic
  switch (opcode.behavior) {
  case UNIMPL: return unimpl_handler(this);
  case STOP: throw std::logic_error("Unimplemented instruction: STOP");
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
