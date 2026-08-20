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
    // One byte holding NZVC in its low nibble, N at bit 3 down to C at bit 0. A byte per flag would cost 4 payload
    // bytes in every trace record that touches the flags, to carry 4 bits.
    cfg.span = {0, 0};
    cfg.skip_serialize = true;
    self->_csrs = sys->make_device<Dense>(parent, cfg);
  };
  sys->make_deferred(DeferredDevice{.parent = _config.id, .ctor = make_csrs});
}

const Target *PepISA3CPU::target() const { return _target; }

void PepISA3CPU::initialize(System *sys) {
  _op_data = Operation(Operation::Type::Standard, Operation::Kind::data, id());

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
  static const auto RO = RegisterScan::Register::Access::Read;
  // Core registers
  using R = isa::Pep10::Register;
  const auto rid = _regbank->id();
  static const auto r2i = [](const R &r) -> u16 { return static_cast<u16>(r) * 2; };
  scan->expose(SR{.byte_width = 2, .guest_access = RW, .target = rid, .order = BE, .name = "A", .loc = r2i(R::A)});
  scan->expose(SR{.byte_width = 2, .guest_access = RW, .target = rid, .order = BE, .name = "X", .loc = r2i(R::X)});
  scan->expose(SR{.byte_width = 2, .guest_access = RW, .target = rid, .order = BE, .name = "PC", .loc = r2i(R::PC)});
  scan->expose(SR{.byte_width = 2, .guest_access = RW, .target = rid, .order = BE, .name = "SP", .loc = r2i(R::SP)});
  scan->expose(SR{.byte_width = 1,
                  .guest_access = RW,
                  .target = rid,
                  .order = BE,
                  .name = "IS",
                  .loc = static_cast<Address>(r2i(R::IS) + 1)});
  scan->expose(SR{.byte_width = 2, .guest_access = RW, .target = rid, .order = BE, .name = "OS", .loc = r2i(R::OS)});
  // CSRs / Flags
  using C = isa::Pep10::CSR;
  const auto cid = _csrs->id();
  using F = SR::Field;
  // Should really be 4 separate fields, but I want to test that my fields work as expected.
  // Bit 7 is MSB, 0 is LSB. The flags occupy the low nibble in CSR enum order, so N is bit 3 and C is bit 0.
  auto n = F{.guest_access = RW, .bit_offset = 3, .bit_width = 1, .name = "N"};
  auto z = F{.guest_access = RW, .bit_offset = 2, .bit_width = 1, .name = "Z"};
  auto v = F{.guest_access = RW, .bit_offset = 1, .bit_width = 1, .name = "V"};
  auto c = F{.guest_access = RW, .bit_offset = 0, .bit_width = 1, .name = "C"};
  scan->expose(SR{.byte_width = 1,
                  .guest_access = RW,
                  .target = cid,
                  .order = BE,
                  .name = "NZVC",
                  .fields = {n, z, v, c},
                  .loc = Address(0)});
  const auto cpuid = id();
  // Expose call depth, which is useful for implementing step modes.
  // host_access defaults to ReadWrite, which is used by the trace buffer for step_back.
  _ref_call_depth = scan->expose(SR{.byte_width = 2,
                                    .guest_access = RO,
                                    .restore_on_step_back = true,
                                    .kind = SR::Kind::Gauge,
                                    .visibility = SR::Visibility::Internal,
                                    .target = cpuid,
                                    .order = bits::hostOrder(),
                                    .name = "call_depth",
                                    .loc = &_count.call_depth});
  // Width must match the storage it points at: the pointer visitors compare sizeof(T) against byte_width.
  scan->expose(SR{.byte_width = sizeof(_count.instructions),
                  .guest_access = RO,
                  .restore_on_step_back = false,
                  .kind = SR::Kind::Count,
                  .visibility = SR::Visibility::Internal,
                  .target = cpuid,
                  .order = bits::hostOrder(),
                  .name = "icount",
                  .loc = &_count.instructions});
}

void PepISA3CPU::reset() {
  _pc = 0;
  _count = {};
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
  const auto init_pc = _pc;
  // TODO: Should probably be an instruction access?
  u8 is = _target->read<u8, false>(_pc, op_data()).second;
  _pc += 1;
  write_register(isa::Pep10::Register::IS, is);
  // Defer PC writeback until end of instruction to avoid ~3 updates on a BR (1 for to fetch IS, 1 to fetch OS, 1 for
  // the branch).
  handle(_opcodes[is]);
  // Change in PC is range [1, 3] which is the normal increment amount and probably not from a branch.
  // Since all instructions other than branches have a fixed PC increment, we can use a specialized increment encoding
  // to save ~2B/instruction in the trace. We do not use the normal encoding for calls/branches, as those can have
  // data-dependence for the branch target.
  const auto pc_delta = _pc - init_pc;
  if ((pc_delta & 0b11) == pc_delta) {
    _regbank->write_increment<u16, bits::host_is_le>(static_cast<u8>(isa::Pep10::Register::PC) * 2, _pc,
                                                     op_data(), bits::Order::BigEndian);
  } else write_register_uncached(isa::Pep10::Register::PC, _pc);
  // TODO: handle breakpoints, debug info, etc
  record.commit();
  _count.instructions += 1;
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
  _count.call_depth += 1;
  // Ordering does not matter here the way it does for a write since the prior is constant.
  _trace.emit_incr_register(op_data(), _ref_call_depth, 1);
}

void PepISA3CPU::decrement_call_depth() {
  _count.call_depth -= 1;
  _trace.emit_incr_register(op_data(), _ref_call_depth, -1);
}

// The CSR bank is one byte holding all four flags, N at bit 3 through C at bit 0, matching the packing at the ISA
// layer. One access also means one trace record byte rather than 4
u8 PepISA3CPU::read_packed_csr() { return static_cast<u8>(_csrs->read<u8, false>(0, op_data()).second & CSR_MASK); }

void PepISA3CPU::write_packed_csr(u8 value) {
  _csrs->write<u8, false>(0, static_cast<u8>(value & CSR_MASK), op_data());
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
