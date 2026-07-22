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

namespace {
static const bool swap = bits::hostOrder() != bits::Order::BigEndian;
static const Operation rw_d{.type = Operation::Type::Standard, .kind = Operation::Kind::data};

// Read word at Mem[PC] and store to OS, incrementing PC by 2.
// Return the /address/ of the operand value, which is usable for both load and store instructions.
// For store-type operands, this is the address you write to. For load-type operands, you will need to read from this
// address to get the actual operand specifier.
u16 decode_op_addr(PepISA3CPU *self, isa::SharedAddrMode addr) {
  // Fetch current PC
  u16 pc = self->read_register(isa::Pep10::Register::PC);
  // Increment PC by 2 to point to next instruction.
  self->write_register(isa::Pep10::Register::PC, pc + 2);
  auto target = self->target();
  // Read value at mem[PC] into OS register.
  u16 opr = target->read<u16, swap>(pc, rw_d).second;
  self->write_register(isa::Pep10::Register::OS, opr);

  switch (addr) {
  case isa::SharedAddrMode::I: return pc;
  case isa::SharedAddrMode::N: opr = target->read<u16, swap>(opr, rw_d).second; [[fallthrough]];
  case isa::SharedAddrMode::D: return opr;

  case isa::SharedAddrMode::SF:
    opr = self->read_register(isa::Pep10::Register::SP) + opr;
    return self->target()->read<u16, swap>(opr, rw_d).second;

  case isa::SharedAddrMode::S: return self->read_register(isa::Pep10::Register::SP) + opr;
  case isa::SharedAddrMode::X: return self->read_register(isa::Pep10::Register::X) + opr;
  case isa::SharedAddrMode::SX:
    return self->read_register(isa::Pep10::Register::X) + self->read_register(isa::Pep10::Register::SP) + opr;
  case isa::SharedAddrMode::SFX:
    opr = self->read_register(isa::Pep10::Register::SP) + opr;
    return self->read_register(isa::Pep10::Register::X) + self->target()->read<u16, swap>(opr, rw_d).second;
  }
  throw std::logic_error("Invalid addressing mode for decode_op_addr");
}

u8 pack_csr(bool n, bool z, bool v, bool c) {
  u8 nzvc = 0;
  if (n) nzvc |= 1 << 0;
  if (z) nzvc |= 1 << 1;
  if (v) nzvc |= 1 << 2;
  if (c) nzvc |= 1 << 3;
  return nzvc;
}

std::tuple<bool, bool, bool, bool> unpack_csrs(u8 nzvc) {
  bool n = nzvc & (1 << 0);
  bool z = nzvc & (1 << 1);
  bool v = nzvc & (1 << 2);
  bool c = nzvc & (1 << 3);
  return {n, z, v, c};
}
using Op = isa::SharedOp;
void unimpl_handler(PepISA3CPU *) { throw std::logic_error("Unimplemented instruction encountered"); }

void handle_ret(PepISA3CPU *self) {
  self->decrement_call_depth();
  u16 sp = self->read_register(isa::Pep10::Register::SP);
  auto addr = self->target()->read<u16, swap>(sp, rw_d).second;
  self->write_register(isa::Pep10::Register::PC, addr);
  // TODO: notify debugger of ret @ PC
}

void handle_movflga(PepISA3CPU *self) {
  auto nzvc = self->read_packed_csr();
  self->write_register(isa::Pep10::Register::A, nzvc);
}

void handle_movaflg(PepISA3CPU *self) {
  auto nzvc = self->read_register(isa::Pep10::Register::A);
  self->write_packed_csr(nzvc);
}

void handle_movspa(PepISA3CPU *self) {
  auto sp = self->read_register(isa::Pep10::Register::SP);
  self->write_register(isa::Pep10::Register::A, sp);
}

void handle_movasp(PepISA3CPU *self) {
  auto a = self->read_register(isa::Pep10::Register::A);
  self->write_register(isa::Pep10::Register::SP, a);
}

void handle_nop(PepISA3CPU *) {}

void handle_negr(PepISA3CPU *self, isa::Pep10::Register reg) {
  u16 src = self->read_register(reg);
  u16 tmp = ~src + 1;
  bool n = tmp & 0x8000;
  bool z = tmp == 0x0000;
  bool v = tmp == 0x8000;
  bool c = src == 0x0000;
  self->write_register(reg, tmp);
  self->write_packed_csr(pack_csr(n, z, v, c));
}

void handle_aslr(PepISA3CPU *self, isa::Pep10::Register reg) {
  auto src = self->read_register(reg);
  // Store in temp, because we need acc for status bit computation.
  u16 tmp = src << 1;
  // Is negative if high order bit is 1.
  bool n = tmp & 0x8000;
  // Is zero if all bits are 0's.
  bool z = tmp == 0x0000;
  // Signed overflow occurs when the starting & ending values of the high
  // order bit differ (a xor temp == 1). Then shift the result over by 15
  // places to only keep high order bit (which is the sign).
  bool v = (src ^ tmp) >> 15;
  // Carry out if register starts with high order 1.
  bool c = src & 0x8000;
  self->write_register(reg, tmp);
  self->write_packed_csr(pack_csr(n, z, v, c));
}

void handle_asrr(PepISA3CPU *self, isa::Pep10::Register reg) {
  auto src = self->read_register(reg);
  // Shift all bits to the right by 1 position. Since using unsigned shift,
  // must explicitly perform sign extension by hand.
  u16 tmp = static_cast<u16>(src >> 1 | ((src & 0x8000) ? 1 << 15 : 0));
  // Is negative if high order bit is 1.
  bool n = tmp & 0x8000;
  // Is zero if all bits are 0's.
  bool z = tmp == 0x000;
  // Carry out if register starts with low order 1.
  bool c = src & 0x1;
  bool v = 0;
  self->write_register(reg, tmp);
  self->write_packed_csr(pack_csr(n, z, v, c));
}

void handle_notr(PepISA3CPU *self, isa::Pep10::Register reg) {
  auto src = self->read_register(reg);
  auto [n, z, v, c] = unpack_csrs(self->read_packed_csr());
  u16 tmp = ~src;
  n = tmp & 0x8000;
  z = tmp == 0x0000;
  self->write_register(reg, tmp);
  self->write_packed_csr(pack_csr(n, z, v, c));
}

void handle_rolr(PepISA3CPU *self, isa::Pep10::Register reg) {
  auto src = self->read_register(reg);
  auto [n, z, v, c] = unpack_csrs(self->read_packed_csr());
  // Shift the carry in to low order bit.
  u16 tmp = static_cast<u16>(src << 1 | (c ? 1 : 0));
  n = tmp & 0x8000;
  z = tmp == 0x0000;
  // Carry out if register starts with high order 1.
  c = src & 0x8000;
  self->write_register(reg, tmp);
  self->write_packed_csr(pack_csr(n, z, v, c));
}

void handle_rorr(PepISA3CPU *self, isa::Pep10::Register reg) {
  auto src = self->read_register(reg);
  auto [n, z, v, c] = unpack_csrs(self->read_packed_csr());
  // Shift the carry in to high order bit.
  u16 tmp = src >> 1 | (c ? 1 << 15 : 0);
  n = tmp & 0x8000;
  z = tmp == 0x0000;
  // Carry out if register starts with low order 1.
  c = src & 0x1;
  self->write_register(reg, tmp);
  self->write_packed_csr(pack_csr(n, z, v, c));
}

void handle_sret(PepISA3CPU *self) {
  // Long enough to either hold all regs or one ctx switch block.
  static constexpr u8 registersBytes = 2 * ::isa::Pep10::RegisterCount;
  u8 ctx[std::max<std::size_t>(registersBytes, 12)];
  auto ctxSpan = bits::span<u8>{ctx, sizeof(ctx)};

  auto memory = self->target();
  // Fill ctx with all register's current values.
  // Then we can do a single write back to _regs and only generate 1 trace
  // packet.
  auto regs = self->registers();
  u16 sp = self->read_register(isa::Pep10::Register::SP);
  u16 tmp = size_inclusive(regs->span());
  regs->read(0, {ctx, tmp}, rw_d);

  // Reload NZVC
  auto csrs = memory->read<u8>(sp, rw_d).second;
  self->write_packed_csr(csrs);

  // Load A into ctx. No need for byteswap, _memory is little endian as are
  // regs.
  memory->read(sp + 1, {ctx + 2 * static_cast<u8>(isa::Pep10::Register::A), 2}, rw_d);

  // Load X into ctx
  memory->read(sp + 3, {ctx + 2 * static_cast<u8>(isa::Pep10::Register::X), 2}, rw_d);

  // Load PC into ctx
  memory->read(sp + 5, {ctx + 2 * static_cast<u8>(isa::Pep10::Register::PC), 2}, rw_d);

  // Load SP into ctx
  memory->read(sp + 7, {ctx + 2 * static_cast<u8>(isa::Pep10::Register::SP), 2}, rw_d);

  // Bulk write-back regs, saving a number of bits on trace metadata.
  regs->write(0, {ctx, registersBytes}, rw_d);

  tmp = sp + 12;
  // Using "host"'s variables, so byte swap if necessary.
  if (swap) tmp = bits::byteswap(tmp);
  memory->write(static_cast<u16>(::isa::Pep10::MemoryVectors::SystemStackPtr), {reinterpret_cast<u8 *>(&tmp), 2}, rw_d);

  self->decrement_call_depth();
  if (false) {
    //_dbg->bps->notifyPCChanged(readReg(Register::PC));
    //_dbg->notifyTrapRet(pc - 1, readReg(Register::SP));
  }
  // Skip "normal" return path, since we've already written to PC.
}

void handle_addr(PepISA3CPU *self, Op op) {
  isa::Pep10::Register reg = static_cast<isa::Pep10::Register>(op.target);
  u16 op_addr = decode_op_addr(self, op.addr);
  u16 operand = self->target()->read<u16, swap>(op_addr, rw_d).second;
  auto src = self->read_register(reg);
  u16 tmp = src + operand;
  // Is negative if high order bit is 1.
  bool n = tmp & 0x8000;
  // Is zero if all bits are 0's.
  bool z = tmp == 0x0000;
  // There is a signed overflow iff the high order bits of the register
  // and operand are the same, and one input & the output differ in sign.
  // >> Shifts in 0's (unsigned shorts), so after shift, only high order
  // bit remain.
  bool v = (~(src ^ operand) & (src ^ tmp)) >> 15;
  // Carry out iff result is unsigned less than register or operand.
  bool c = tmp < src || tmp < operand;
  self->write_register(reg, tmp);
  self->write_packed_csr(pack_csr(n, z, v, c));
}

void handle_subr(PepISA3CPU *self, Op op) {
  const isa::Pep10::Register reg = static_cast<isa::Pep10::Register>(op.target);
  u16 op_addr = decode_op_addr(self, op.addr);
  u16 operand = self->target()->read<u16, swap>(op_addr, rw_d).second;
  auto src = self->read_register(reg);
  u16 tmp = src + ~operand + 1;
  // Is negative if high order bit is 1.
  bool n = tmp & 0x8000;
  // Is zero if all bits are 0's.
  bool z = tmp == 0x0000;
  // There is a signed overflow iff the high order bits of the register
  // and operand are the same, and one input & the output differ in sign.
  // >> Shifts in 0's (unsigned shorts), so after shift, only high order
  // bit remain.
  bool v = (~(src ^ (~operand + 1)) & (src ^ tmp)) >> 15;
  // Carry out iff result is unsigned less than register or operand.
  bool c = tmp < src || tmp < static_cast<u16>(1 + ~operand);
  self->write_register(reg, tmp);
  self->write_packed_csr(pack_csr(n, z, v, c));
}

enum class Bitop {
  AND,
  OR,
  XOR,
};

void handle_bitopr(PepISA3CPU *self, Op op, Bitop bitop) {
  const isa::Pep10::Register reg = static_cast<isa::Pep10::Register>(op.target);
  auto [n, z, v, c] = unpack_csrs(self->read_packed_csr());
  u16 op_addr = decode_op_addr(self, op.addr);
  u16 operand = self->target()->read<u16, swap>(op_addr, rw_d).second;
  auto src = self->read_register(reg);
  u16 tmp;
  switch (bitop) {
  case Bitop::AND: tmp = src & operand; break;
  case Bitop::OR: tmp = src | operand; break;
  case Bitop::XOR: tmp = src ^ operand; break;
  }
  // Is negative if high order bit is 1.
  n = tmp & 0x8000;
  // Is zero if all bits are 0's.
  z = tmp == 0x0000;
  self->write_register(reg, tmp);
  self->write_packed_csr(pack_csr(n, z, v, c));
}

void handle_ldwr(PepISA3CPU *self, Op op) {
  const isa::Pep10::Register reg = static_cast<isa::Pep10::Register>(op.target);
  u16 op_addr = decode_op_addr(self, op.addr);
  u16 operand = self->target()->read<u16, swap>(op_addr, rw_d).second;
  auto [n, z, v, c] = unpack_csrs(self->read_packed_csr());
  // Is negative if high order bit is 1.
  n = operand & 0x8000;
  z = operand == 0x0000;

  self->write_register(reg, operand);
  self->write_packed_csr(pack_csr(n, z, v, c));
}

void handle_ldbr(PepISA3CPU *self, Op op) {
  const isa::Pep10::Register reg = static_cast<isa::Pep10::Register>(op.target);
  u16 op_addr = decode_op_addr(self, op.addr);
  u8 operand = self->target()->read<u8>(op_addr, rw_d).second;
  auto [n, z, v, c] = unpack_csrs(self->read_packed_csr());
  // LDBr always clears n.
  n = 0;
  z = operand == 0x0000;

  self->write_register(reg, operand);
  self->write_packed_csr(pack_csr(n, z, v, c));
}

void handle_stwr(PepISA3CPU *self, Op op) {
  const isa::Pep10::Register reg = static_cast<isa::Pep10::Register>(op.target);
  u16 op_addr = decode_op_addr(self, op.addr);
  u16 src = self->read_register(reg);
  self->target()->write<u16, swap>(op_addr, src, rw_d);
}

void handle_stbr(PepISA3CPU *self, Op op) {
  const isa::Pep10::Register reg = static_cast<isa::Pep10::Register>(op.target);
  u16 op_addr = decode_op_addr(self, op.addr);
  u8 src = self->read_register(reg);
  self->target()->write<u8>(op_addr, src, rw_d);
}

void handle_cpwr(PepISA3CPU *self, Op op) {
  const isa::Pep10::Register reg = static_cast<isa::Pep10::Register>(op.target);
  u16 op_addr = decode_op_addr(self, op.addr);
  u16 operand = self->target()->read<u16, swap>(op_addr, rw_d).second;
  auto src = self->read_register(reg);
  u16 neg = ~operand + 1;
  u16 tmp = src + neg;
  // Is negative if high order bit is 1.
  bool n = tmp & 0x8000;
  // Is zero if all bits are 0's.
  bool z = tmp == 0x0000;
  // There is a signed overflow iff the high order bits of the register
  // and operand are the same, and one input & the output differ in sign.
  // >> Shifts in 0's (unsigned shorts), so after shift, only high order
  // bit remain.
  bool v = (~(src ^ neg) & (src ^ tmp)) >> 15;
  // Carry out iff result is unsigned less than register or operand.
  bool c = tmp < src || tmp < neg;
  // Invert N bit if there was signed overflow.
  n ^= v;
  self->write_packed_csr(pack_csr(n, z, v, c));
}

void handle_cpbr(PepISA3CPU *self, Op op) {
  const isa::Pep10::Register reg = static_cast<isa::Pep10::Register>(op.target);
  u16 op_addr = decode_op_addr(self, op.addr);
  u8 operand = self->target()->read<u8>(op_addr, rw_d).second;
  auto src = self->read_register(reg);
  // The result is the decoded operand specifier plus A/X. mask down to a byte.
  u16 tmp = (src + ~operand + 1) & 0xff;
  // Is negative if high order bit is 1.
  bool n = tmp & 0x80;
  // Is zero if all bits are 0's.
  bool z = tmp == 0x00;
  // RTL specifies that VC get 0.
  self->write_packed_csr(pack_csr(n, z, 0, 0));
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
  switch (opcode.behavior) {
  case isa::SharedOpBehavior::UNIMPL: return unimpl_handler(this);
  case isa::SharedOpBehavior::RET: return handle_ret(this);
  case isa::SharedOpBehavior::SRET: return handle_sret(this);
  case isa::SharedOpBehavior::MOVFLGA: return handle_movflga(this);
  case isa::SharedOpBehavior::MOVAFLG: return handle_movaflg(this);
  case isa::SharedOpBehavior::MOVSPA: return handle_movspa(this);
  case isa::SharedOpBehavior::MOVASP: return handle_movasp(this);
  case isa::SharedOpBehavior::HW_NOP: return handle_nop(this);
  case isa::SharedOpBehavior::NEG: return handle_negr(this, (R)opcode.target);
  case isa::SharedOpBehavior::ASL: return handle_aslr(this, (R)opcode.target);
  case isa::SharedOpBehavior::ASR: return handle_asrr(this, (R)opcode.target);
  case isa::SharedOpBehavior::NOT: return handle_notr(this, (R)opcode.target);
  case isa::SharedOpBehavior::ROL: return handle_rolr(this, (R)opcode.target);
  case isa::SharedOpBehavior::ROR: return handle_rorr(this, (R)opcode.target);
  case isa::SharedOpBehavior::BR: break;
  case isa::SharedOpBehavior::BRLE: break;
  case isa::SharedOpBehavior::BRLT: break;
  case isa::SharedOpBehavior::BREQ: break;
  case isa::SharedOpBehavior::BRNE: break;
  case isa::SharedOpBehavior::BRGE: break;
  case isa::SharedOpBehavior::BRGT: break;
  case isa::SharedOpBehavior::BRV: break;
  case isa::SharedOpBehavior::BRC: break;
  case isa::SharedOpBehavior::CALL: break;
  case isa::SharedOpBehavior::SCALL: break;
  case isa::SharedOpBehavior::TRAP_CALL: break;
  case isa::SharedOpBehavior::ADDSP: break;
  case isa::SharedOpBehavior::SUBSP: break;
  case isa::SharedOpBehavior::ADD: return handle_addr(this, opcode);
  case isa::SharedOpBehavior::SUB: return handle_subr(this, opcode);
  case isa::SharedOpBehavior::AND: return handle_bitopr(this, opcode, Bitop::AND);
  case isa::SharedOpBehavior::OR: return handle_bitopr(this, opcode, Bitop::OR);
  case isa::SharedOpBehavior::XOR: return handle_bitopr(this, opcode, Bitop::XOR);
  case isa::SharedOpBehavior::CPW: return handle_cpwr(this, opcode);
  case isa::SharedOpBehavior::CPB: return handle_cpbr(this, opcode);
  case isa::SharedOpBehavior::LDW: return handle_ldwr(this, opcode);
  case isa::SharedOpBehavior::LDB: return handle_ldbr(this, opcode);
  case isa::SharedOpBehavior::STW: return handle_stwr(this, opcode);
  case isa::SharedOpBehavior::STB: return handle_stbr(this, opcode);
  default: throw std::logic_error("Unknown opcode behavior");
  }
}
Target *PepISA3CPU::target() { return _target; }
