#include "register_blaster.hpp"
#include <algorithm>
#include <bit>
#include "core/sim/api/memory.hpp"
#include "core/sim/debugger/register_scanner.hpp"
#include "core/sim/debugger/tvm_tracebuffer.hpp"
#include "core/sim/memory/errors.hpp"
#include "core/sim/system.hpp"
#include "register_scanner.hpp"

namespace {
const Operation rw_cmp(Operation::Type::BufferInternal, Operation::Kind::data);

// Run a target acccess and report if it succeded, used to catch bad access from targets and set the F bit accordingly.
template <typename Fn> bool try_access(Fn &&fn) {
  try {
    fn();
    return true;
  } catch (const Error &) {
    return false;
  }
}
} // namespace

RegisterBlaster::RegisterBlaster(std::shared_ptr<pepp::bts::BufferManager> mgr, System *system)
    : _mgr(mgr), _system(system) {
  if (_system) _scan = _system->register_scan();
  else _scan = nullptr;
}

void RegisterBlaster::update_ip(pepp::bts::Buffer::Location loc) { return update_ip(loc.id, loc.offset); }

void RegisterBlaster::update_ip(pepp::bts::Buffer::ID id, u16 offset) {
  _regs.IP.hi = id.value;
  _regs.IP.lo = offset & 0xFFFE;
}

void RegisterBlaster::step() {
  if (_csrs.CLRMOD) {
    _regs.MOD1 = {};
    _regs.MOD2 = {};
    _csrs.M1 = 0, _csrs.M2 = 0;
  }
  decode();
  if (_csrs.L) execute();
}

void RegisterBlaster::run_direct(pepp::bts::Buffer::Location loc) {
  // Bring the blaster back into the live state (L==1).
  // Clear F bit in case last program terminated with hardfail.
  // "soft stop" is L==0,F==0, "hard stop is L==0,F==1.
  _csrs.L = 1, _csrs.F = 0;
  update_ip(loc);
  while (_csrs.L) step();
}

void RegisterBlaster::run_indirect(std::span<pepp::bts::Buffer::Location> locs) {
  // Run the program at each location. Check for a hard stop condition. On hard stop, abort the loop.
  // On a normal/soft stop, resume execution of the next program.
  for (const auto &loc : locs)
    if (run_direct(loc); _csrs.F == 1) break;
}

void RegisterBlaster::soft_stop(tvm::StopCause cause) {
  _csrs.L = 0;
  _csrs.F = 0;
  _regs.STOP_CAUSE = cause;
}

void RegisterBlaster::hard_stop(tvm::StopCause cause) {
  _csrs.L = 0;
  _csrs.F = 1;
  _regs.STOP_CAUSE = cause;
}

void RegisterBlaster::decode() {
  using namespace tvm;
  const auto ibp = (pepp::bts::Buffer::ID)_regs.IP.hi;
  // read 16 bits at ip.lo from data and increment.
  u16 opcode = read16(ibp, _regs.IP.lo);
  // Perform bit-cracking to expose fields
  _regs.IS = tvm::OpWord(opcode);
  _csrs.CLRMOD = _regs.IS.clrmod;
  // Whenever a packet needs an a refernece relative to the IP's offset into the current buffer,
  // refer to this variable that than IP. This lets us pre-increment IP and avoid difficulties with branching / early
  // returns.
  const auto iop = 2 + _regs.IP.lo;

  // Mask out low-order bit, because opcodes are naturally aligned.
  _regs.IP.lo = (_regs.IP.lo + 2 + _regs.IS.word_len * 2) & 0xFFFE;
  switch (static_cast<Opcode>(_regs.IS.ocpode)) {
  case Opcode::HALT: _decoded = decode_halt(ibp, iop); break;
  case Opcode::RET: _decoded = decode_ret(ibp, iop); break;
  case Opcode::CALL: _decoded = decode_call(ibp, iop); break;
  case Opcode::INVCALL: _decoded = decode_invcall(ibp, iop); break;
  case Opcode::ASYN: _decoded = decode_asyn(ibp, iop); break;
  case Opcode::ISYN: _decoded = decode_isyn(ibp, iop); break;
  case Opcode::LMR: _decoded = decode_lmr(ibp, iop); break;
  case Opcode::BRF: [[fallthrough]];
  case Opcode::NOP: [[fallthrough]];
  case Opcode::BREQ: [[fallthrough]];
  case Opcode::BRGT: [[fallthrough]];
  case Opcode::BRGE: [[fallthrough]];
  case Opcode::BRLT: [[fallthrough]];
  case Opcode::BRLE: [[fallthrough]];
  case Opcode::BRNE: [[fallthrough]];
  case Opcode::BR: _decoded = decode_br(ibp, iop); break;
  case Opcode::SETMEM: [[fallthrough]]; // Difference between SETMEM/X is in execution, not decoding
  case Opcode::SETMEMX: _decoded = decode_setmem(ibp, iop); break;
  case Opcode::CMPMEM: _decoded = decode_cmpmem(ibp, iop); break;
  case Opcode::CLRMEM: _decoded = decode_clrmem(ibp, iop); break;
  case Opcode::SETREG: [[fallthrough]]; // Difference between SETREG/X is in execution, not decoding
  case Opcode::SETREGX: _decoded = decode_setreg(ibp, iop); break;
  case Opcode::CMPREG: _decoded = decode_cmpreg(ibp, iop); break;
  case Opcode::CLRREG: _decoded = decode_clrreg(ibp, iop); break;
  case Opcode::TRADDR: _decoded = decode_traddr(ibp, iop); break;
  case Opcode::LDP: _decoded = decode_ldp(ibp, iop); break;
  case Opcode::ACCDP: _decoded = decode_accdp(ibp, iop); break;
  case Opcode::INCDP: _decoded = decode_incdp(ibp, iop); break;
  default: hard_stop(StopCause::IllegalOpcode); break; // Treat unrecognized upcodes as hard failures.
  }
}

tvm::DecodedOp::Halt RegisterBlaster::decode_halt(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::Halt ret;
  switch (_regs.IS.word_len) {
  default: [[fallthrough]];
  case 1: ret.cause = (tvm::StopCause)read16(ibp, iop + 2); break;
  case 0: ret.cause = tvm::StopCause::None; break;
  }
  return ret;
}

tvm::DecodedOp::Ret RegisterBlaster::decode_ret(pepp::bts::Buffer::ID ibp, u16 iop) {
  // No-op for decoding, since all data is passed on stack.
  return {};
}

tvm::DecodedOp::Call RegisterBlaster::decode_call(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::Call ret;
  ret.next_ip.hi = _regs.IP.hi, ret.next_ip.lo = iop + 0;
  switch (_regs.IS.word_len) {
  default: [[fallthrough]];
  case 2: ret.next_ip.hi = read16(ibp, iop + 2); [[fallthrough]];
  case 1: ret.next_ip.lo = read16(ibp, iop + 0);
  case 0: break;
  }
  return ret;
}

tvm::DecodedOp::InvCall RegisterBlaster::decode_invcall(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::InvCall ret;
  // IP.lo has already been advanced past this packet, so IP is the fall-through address. Any target word the packet
  // omits defaults to it: a missing hi word keeps IP.hi, and a wholly missing target calls the next instruction.
  ret.on_true = ret.on_false = _regs.IP;
  // Targets interleave lo-first, so a near call (both targets in this buffer) costs 2 words instead of 4.
  switch (_regs.IS.word_len) {
  default: [[fallthrough]];
  case 4: ret.on_false.hi = read16(ibp, iop + 6); [[fallthrough]];
  case 3: ret.on_true.hi = read16(ibp, iop + 4); [[fallthrough]];
  case 2: ret.on_false.lo = read16(ibp, iop + 2); [[fallthrough]];
  case 1: ret.on_true.lo = read16(ibp, iop + 0); [[fallthrough]];
  case 0: break;
  }
  // Mirror resolved targets into modifier registers if appropriate.
  if (_regs.IS.word_len >= 1) _regs.MOD1 = ret.on_true, _csrs.M1 = 1;
  if (_regs.IS.word_len >= 2) _regs.MOD2 = ret.on_false, _csrs.M2 = 1;
  return ret;
}

u64 RegisterBlaster::decode_syn_data(pepp::bts::Buffer::ID ibp, u16 iop, u8 &size) {
  using StopCause = tvm::StopCause;
  // Unless a size word is provided, data is DP relative rather than immediate.
  tvm::SegmentPair data = _regs.DP;
  // DS is shared with the SET*/CMP* ops, so it may well be wider than a timestamp. Ignore the excess.
  size = (u8)std::min<u16>(_regs.DS, sizeof(u64));

  if (_regs.IS.word_len >= 1) {
    // If MOD1/MOD2 are set, then read from them rather than the data registers.
    // This allows "immediate" versions to avoid clobbering DP regs.
    // Silently truncate to 8 bytes, since our timestamps are at most u64s.
    _regs.MOD1.lo = std::min<u16>(read16(ibp, iop + 0), sizeof(u64));
    _regs.MOD2.hi = _regs.IP.hi;
    _regs.MOD2.lo = iop + 2;
    _csrs.M1 = _csrs.M2 = 1;
    data = _regs.MOD2;
    size = _regs.MOD1.lo;
  }

  u64 value = 0;
  // Ensure that dbuff exists and is in range before reading from it.
  if (auto dbuff = _mgr->find((pepp::bts::Buffer::ID)data.hi); !dbuff) return hard_stop(StopCause::InvalidDBuffer), 0;
  else if (auto span = dbuff->span(); (size_t)data.lo + size > span.size())
    return hard_stop(StopCause::InvalidDBuffer), 0;
  else {
    for (u8 i = 0; i < size; ++i) value |= (u64)span[data.lo + i] << (8 * i);
  }

  return value;
}

tvm::DecodedOp::ASyn RegisterBlaster::decode_asyn(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::ASyn ret;
  u8 width = 0;
  // Absolute timestamps are unsigned, so a narrow encoding is just the low-order bytes of a bigger number.
  ret.timestamp = decode_syn_data(ibp, iop, width);
  return ret;
}

tvm::DecodedOp::ISyn RegisterBlaster::decode_isyn(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::ISyn ret;
  u8 width = 0;
  u64 raw = decode_syn_data(ibp, iop, width);
  // Need to sign-extend the value if it is narrows than 8 bytes.
  if (width > 0 && width < sizeof(u64)) {
    const u64 bits = 8 * (u64)width;
    // Mask all bits except sign bit
    const u64 raw_sign = (u64(1) << (bits - 1));
    // Create a mask for all bits above sign bit. Note the parentheses are different than above!
    const u64 raw_se = ~((u64(1) << bits) - 1);
    if (raw & raw_sign) raw |= raw_se;
  }
  ret.delta = (i64)raw;
  return ret;
}

tvm::DecodedOp::LMR RegisterBlaster::decode_lmr(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::LMR ret;
  if (_regs.IS.word_len == 0) return ret;
  ret.mask = (tvm::RegMask)read16(ibp, iop + 0);
  auto count = _regs.IS.word_len - 1;
  auto buf = _mgr->find(ibp);
  if (buf == nullptr) return ret;
  ret.data = buf->span().subspan(iop + 2, count * 2);
  return ret;
}

tvm::DecodedOp::BR RegisterBlaster::decode_br(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::BR ret;
  // Select condition code base on the original opcode.
  if (_regs.IS.ocpode == (u8)tvm::Opcode::BRF) ret.condition = tvm::ConditionCode::F;
  else ret.condition = (tvm::ConditionCode)(_regs.IS.ocpode & 0x7);
  _regs.MOD1.lo = (u16)ret.condition, _csrs.M1 = 1;

  switch (_regs.IS.word_len) {
  default: [[fallthrough]];
  case 2:
    ret.displacement.hi = read16(ibp, iop + 2);
    ret.displacement.lo = read16(ibp, iop + 0);
    _regs.MOD2 = ret.displacement, _csrs.M2 = 1;
    break;
  case 1:
    ret.displacement.hi = _regs.IP.hi;
    ret.displacement.lo = read16(ibp, iop + 0);
    _regs.MOD2 = ret.displacement, _csrs.M2 = 1;
    break;
  case 0:
    // try to use M2 (if set) as the displacement. If unset, use a 0-displacement from the current op
    if (_csrs.M2) ret.displacement = _regs.MOD2;
    else ret.displacement = {_regs.IP.hi, 0};
    // If MOD2.hi is zero, then treat the displacement as relative to this buffer.
    if (ret.displacement.hi == 0) ret.displacement.hi = _regs.IP.hi;
    break;
  }
  return ret;
}

tvm::DecodedOp::SetMem RegisterBlaster::decode_setmem(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::SetMem ret;
  ret.xor_encoded = (_regs.IS.ocpode == (u8)tvm::Opcode::SETMEMX);
  _csrs.TR = 0; // Enter target mode.
  // Unless (5) is provided, data is DP relative rather than immediate
  ret.data = _regs.DP;
  ret.size = _regs.DS;

  switch (_regs.IS.word_len) {
  default: [[fallthrough]];
  case 5:
    // If MOD1/MOD2 are set, then read from them rather than the data registers.
    // This allows "immediate" versions to avoid clobbering DP regs.
    _regs.MOD1.lo = read16(ibp, iop + 8);
    _regs.MOD2.hi = _regs.IP.hi;
    _regs.MOD2.lo = iop + 10;
    _csrs.M1 = _csrs.M2 = 1;
    ret.data = _regs.MOD2;
    ret.size = _regs.MOD1.lo;
    [[fallthrough]];
  case 4: _regs.OFF.lo = read16(ibp, iop + 6); [[fallthrough]];
  case 3: _regs.OFF.hi = read16(ibp, iop + 4); [[fallthrough]];
  case 2: _regs.ID.lo = read16(ibp, iop + 2); [[fallthrough]];
  case 1: _regs.ACCESS = read16(ibp, iop + 0); [[fallthrough]];
  case 0: break;
  }
  ret.access = _regs.ACCESS;
  ret.target = (Device::ID)_regs.ID.lo;
  ret.offset = _regs.OFF.as_u32();
  return ret;
}

tvm::DecodedOp::CmpMem RegisterBlaster::decode_cmpmem(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::CmpMem ret;
  _csrs.TR = 0; // Enter target mode.
  // Unless (4) is provided, data is DP relative rather than immediate
  ret.data = _regs.DP;
  ret.size = _regs.DS;

  switch (_regs.IS.word_len) {
  default: [[fallthrough]];
  case 4:
    // If MOD1/MOD2 are set, then read from them rather than the data registers.
    // This allows "immediate" versions to avoid clobbering DP regs.
    _regs.MOD1.lo = read16(ibp, iop + 6);
    _regs.MOD2.hi = _regs.IP.hi;
    _regs.MOD2.lo = iop + 8;
    _csrs.M1 = _csrs.M2 = 1;
    ret.data = _regs.MOD2;
    ret.size = _regs.MOD1.lo;
    [[fallthrough]];
  case 3: _regs.OFF.lo = read16(ibp, iop + 4); [[fallthrough]];
  case 2: _regs.OFF.hi = read16(ibp, iop + 2); [[fallthrough]];
  case 1: _regs.ID.lo = read16(ibp, iop + 0); [[fallthrough]];
  case 0: break;
  }
  ret.target = (Device::ID)_regs.ID.lo;
  ret.offset = _regs.OFF.as_u32();
  return ret;
}

tvm::DecodedOp::ClrMem RegisterBlaster::decode_clrmem(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::ClrMem ret;
  _csrs.TR = 0; // Enter target mode.
  ret.data = 0;

  switch (_regs.IS.word_len) {
  default: [[fallthrough]];
  case 2: ret.data = _regs.MOD1.lo = read16(ibp, iop + 2), _csrs.M1 = 1; [[fallthrough]];
  case 1: _regs.ID.lo = read16(ibp, iop + 0); [[fallthrough]];
  case 0: break;
  }
  ret.target = (Device::ID)_regs.ID.lo;
  return ret;
}

tvm::DecodedOp::SetReg RegisterBlaster::decode_setreg(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::SetReg ret;
  ret.xor_encoded = (_regs.IS.ocpode == (u8)tvm::Opcode::SETREGX);
  _csrs.TR = 1; // Enter register mode.
  // Unless (4) is provided, data is DP relative rather than immediate
  ret.data = _regs.DP;
  ret.size = _regs.DS;

  switch (_regs.IS.word_len) {
  default: [[fallthrough]];
  case 4:
    // If MOD1/MOD2 are set, then read from them rather than the data registers.
    // This allows "immediate" versions to avoid clobbering DP regs.
    _regs.MOD1.lo = read16(ibp, iop + 6);
    _regs.MOD2.hi = _regs.IP.hi;
    _regs.MOD2.lo = iop + 8;
    _csrs.M1 = _csrs.M2 = 1;
    ret.data = _regs.MOD2;
    ret.size = _regs.MOD1.lo;
    [[fallthrough]];
  case 3: _regs.ID.lo = read16(ibp, iop + 4); [[fallthrough]];
  case 2: _regs.ID.hi = read16(ibp, iop + 2); [[fallthrough]];
  case 1: _regs.ACCESS = read16(ibp, iop + 0); [[fallthrough]];
  case 0: break;
  }
  ret.access = _regs.ACCESS;
  ret.reg = RegisterScan::RegisterRef{RegisterScan::Register::ID{_regs.ID.hi},
                                      RegisterScan::Register::Field::ID{_regs.ID.lo}};
  return ret;
}

tvm::DecodedOp::CmpReg RegisterBlaster::decode_cmpreg(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::CmpReg ret;
  _csrs.TR = 1; // Enter register mode.
  // Unless (3) is provided, data is DP relative rather than immediate
  ret.data = _regs.DP;
  ret.size = _regs.DS;

  switch (_regs.IS.word_len) {
  default: [[fallthrough]];
  case 3:
    // If MOD1/MOD2 are set, then read from them rather than the data registers.
    // This allows "immediate" versions to avoid clobbering DP regs.
    _regs.MOD1.lo = read16(ibp, iop + 4);
    _regs.MOD2.hi = _regs.IP.hi;
    _regs.MOD2.lo = iop + 6;
    _csrs.M1 = _csrs.M2 = 1;
    ret.data = _regs.MOD2;
    ret.size = _regs.MOD1.lo;
    [[fallthrough]];
  case 2: _regs.ID.lo = read16(ibp, iop + 2); [[fallthrough]];
  case 1: _regs.ID.hi = read16(ibp, iop + 0); [[fallthrough]];
  case 0: break;
  }
  ret.reg = RegisterScan::RegisterRef{RegisterScan::Register::ID{_regs.ID.hi},
                                      RegisterScan::Register::Field::ID{_regs.ID.lo}};
  return ret;
}

tvm::DecodedOp::ClrReg RegisterBlaster::decode_clrreg(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::ClrReg ret;
  _csrs.TR = 1; // Enter register mode.
  switch (_regs.IS.word_len) {
  default: [[fallthrough]];
  case 2: _regs.ID.lo = read16(ibp, iop + 2); [[fallthrough]];
  case 1: _regs.ID.hi = read16(ibp, iop + 0); [[fallthrough]];
  case 0: break;
  }
  ret.reg = RegisterScan::RegisterRef{RegisterScan::Register::ID{_regs.ID.hi},
                                      RegisterScan::Register::Field::ID{_regs.ID.lo}};
  return ret;
}

tvm::DecodedOp::TRADDR RegisterBlaster::decode_traddr(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::TRADDR ret;
  _csrs.TR = 0; // Enter target mode

  switch (_regs.IS.word_len) {
  default: [[fallthrough]];
  case 8: _regs.MOD1.hi = read16(ibp, iop + 14); [[fallthrough]];
  case 7: _regs.MOD1.lo = read16(ibp, iop + 12), _csrs.M1 = 1; [[fallthrough]];
  case 6: _regs.MOD2.lo = read16(ibp, iop + 10); [[fallthrough]];
  case 5: _regs.MOD2.hi = read16(ibp, iop + 8), _csrs.M2 = 1; [[fallthrough]];
  case 4: _regs.ID.hi = read16(ibp, iop + 6); [[fallthrough]];
  case 3: _regs.OFF.lo = read16(ibp, iop + 4); [[fallthrough]];
  case 2: _regs.OFF.hi = read16(ibp, iop + 2); [[fallthrough]];
  case 1: _regs.ID.lo = read16(ibp, iop + 0); [[fallthrough]];
  case 0: break;
  }

  ret.target = (Device::ID)_regs.ID.lo;
  ret.target_offset = _regs.OFF.as_u32();
  ret.source = (Device::ID)_regs.ID.hi;
  ret.source_offset = _regs.MOD2.as_u32();
  ret.size = _regs.MOD1.as_u32();
  return ret;
}

tvm::DecodedOp::LDP RegisterBlaster::decode_ldp(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::LDP ret;
  switch (_regs.IS.word_len) {
  default: [[fallthrough]];
  case 3: _regs.DP.hi = read16(ibp, iop + 4); [[fallthrough]];
  case 2: _regs.DS = read16(ibp, iop + 2); [[fallthrough]];
  case 1: _regs.DP.lo = read16(ibp, iop + 0); [[fallthrough]];
  case 0: break;
  }
  return ret;
}

tvm::DecodedOp::DPIncr RegisterBlaster::decode_accdp(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::DPIncr ret;
  ret.DS = ret.dp_incr = _regs.DS;
  switch (_regs.IS.word_len) {
  default: [[fallthrough]];
  case 1: ret.DS = read16(ibp, iop + 0);
  case 0: break;
  }
  return ret;
}

tvm::DecodedOp::DPIncr RegisterBlaster::decode_incdp(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::DPIncr ret;
  ret.DS = _regs.DS;
  ret.dp_incr = 0;
  switch (_regs.IS.word_len) {
  default: [[fallthrough]];
  case 2: ret.DS = read16(ibp, iop + 2);
  case 1: ret.dp_incr = read16(ibp, iop + 0);
  case 0: break;
  }
  return ret;
}

void RegisterBlaster::execute() {
  using namespace tvm;
  // For instructions which don't just program registers, insert their behaviors here
  switch (static_cast<Opcode>(_regs.IS.ocpode)) {
  case Opcode::HALT: return execute_halt(std::get<tvm::DecodedOp::Halt>(_decoded));
  case Opcode::RET: return execute_ret(std::get<tvm::DecodedOp::Ret>(_decoded));
  case Opcode::CALL: return execute_call(std::get<tvm::DecodedOp::Call>(_decoded));
  case Opcode::INVCALL: return execute_invcall(std::get<tvm::DecodedOp::InvCall>(_decoded));
  case Opcode::ASYN: return execute_asyn(std::get<tvm::DecodedOp::ASyn>(_decoded));
  case Opcode::ISYN: return execute_isyn(std::get<tvm::DecodedOp::ISyn>(_decoded));
  case Opcode::LMR: return execute_lmr(std::get<tvm::DecodedOp::LMR>(_decoded));
  case Opcode::BRF: [[fallthrough]];
  case Opcode::NOP: [[fallthrough]];
  case Opcode::BREQ: [[fallthrough]];
  case Opcode::BRGT: [[fallthrough]];
  case Opcode::BRGE: [[fallthrough]];
  case Opcode::BRLT: [[fallthrough]];
  case Opcode::BRLE: [[fallthrough]];
  case Opcode::BRNE: [[fallthrough]];
  case Opcode::BR: return execute_br(std::get<tvm::DecodedOp::BR>(_decoded));
  case Opcode::SETMEM: [[fallthrough]]; // Difference between SETMEM/X is in execution, not decoding
  case Opcode::SETMEMX: return execute_setmem(std::get<tvm::DecodedOp::SetMem>(_decoded));
  case Opcode::CMPMEM: return execute_cmpmem(std::get<tvm::DecodedOp::CmpMem>(_decoded));
  case Opcode::CLRMEM: return execute_clrmem(std::get<tvm::DecodedOp::ClrMem>(_decoded));
  case Opcode::SETREG: [[fallthrough]]; // Difference between SETREG/X is in execution, not decoding
  case Opcode::SETREGX: return execute_setreg(std::get<tvm::DecodedOp::SetReg>(_decoded));
  case Opcode::CMPREG: return execute_cmpreg(std::get<tvm::DecodedOp::CmpReg>(_decoded));
  case Opcode::CLRREG: return execute_clrreg(std::get<tvm::DecodedOp::ClrReg>(_decoded));
  case Opcode::TRADDR: return execute_traddr(std::get<tvm::DecodedOp::TRADDR>(_decoded));
  case Opcode::LDP: return execute_ldp(std::get<tvm::DecodedOp::LDP>(_decoded));
  case Opcode::ACCDP: [[fallthrough]];
  case Opcode::INCDP: return execute_dpincr(std::get<tvm::DecodedOp::DPIncr>(_decoded));
  default: hard_stop(StopCause::IllegalOpcode); break;
  }
}

void RegisterBlaster::execute_halt(tvm::DecodedOp::Halt op) { soft_stop(op.cause); }

void RegisterBlaster::execute_ret(tvm::DecodedOp::Ret) { _regs.IP = pop(); }

void RegisterBlaster::execute_call(tvm::DecodedOp::Call op) {
  push(_regs.IP);
  _regs.IP = op.next_ip;
}

void RegisterBlaster::execute_invcall(tvm::DecodedOp::InvCall op) {
  push(_regs.IP);
  // F is deliberately left as-is: the callee is the one that wants to know whether it was reached because of a
  // failure, and clearing it here would hide that from a subsequent BRF.
  _regs.IP = _csrs.F ? op.on_true : op.on_false;
}

// Both sync ops are no-ops for the blaster itself, which keeps no clock. All the work happened in decode, and the
// timestamp is carried in the decoded op for inspection code sitting between the decode and execute stages.
void RegisterBlaster::execute_asyn(tvm::DecodedOp::ASyn op) {}

void RegisterBlaster::execute_isyn(tvm::DecodedOp::ISyn op) {}

void RegisterBlaster::execute_lmr(tvm::DecodedOp::LMR op) {
  using namespace tvm;
  u16 mask = (u16)op.mask;
  auto pos = 0;
  while (mask != 0 && pos < op.word_count()) {
    // Position of lowest set bit.
    int i = std::countr_zero(mask); // position of lowest set bit, 0..13
    // Mask for that single bit.
    RegMask masked = (RegMask)(1u << i);
    mask &= mask - 1; // Clear lowest bit

    u16 temp = op.word(pos++);

    switch (masked) {
    case RegMask::IP_HI: _regs.IP.hi = temp; break;
    case RegMask::IP_LO: _regs.IP.lo = temp & 0xFFFE; break; // Must mask low order bit to force IP to be word-aligned.
    case RegMask::DP_HI: _regs.DP.hi = temp; break;
    case RegMask::DP_LO: _regs.DP.lo = temp; break;
    case RegMask::DS: _regs.DS = temp; break;
    case RegMask::ACCESS: _regs.ACCESS = temp; break;
    case RegMask::ID_HI: _regs.ID.hi = temp; break;
    case RegMask::ID_LO: _regs.ID.lo = temp; break;
    case RegMask::OFF_HI: _regs.OFF.hi = temp; break;
    case RegMask::OFF_LO: _regs.OFF.lo = temp; break;
    case RegMask::MOD1_HI: _regs.MOD1.hi = temp, _csrs.M1 = 1; break;
    case RegMask::MOD1_LO: _regs.MOD1.lo = temp, _csrs.M1 = 1; break;
    case RegMask::MOD2_HI: _regs.MOD2.hi = temp, _csrs.M2 = 1; break;
    case RegMask::MOD2_LO: _regs.MOD2.lo = temp, _csrs.M2 = 1; break;
    case RegMask::FLAGS: {
      bool old_l = _csrs.L;
      _csrs = Flags(temp & 0xFF);
      _csrs.L = old_l;
      break;
    }
    default: break; // Ignore unknown bits. This allows future expansion of the mask without breaking old blasters.
    }
  }
}

void RegisterBlaster::execute_br(tvm::DecodedOp::BR op) {
  using namespace bits;
  using CC = tvm::ConditionCode;
  const u16 cc = ((u16)op.condition) & (u16)CC::MASK;
  const bool pass_e = _csrs.Z && (cc & (u16)CC::E);
  const bool pass_l = _csrs.N && (cc & (u16)CC::L);
  const bool pass_g = !_csrs.N && !_csrs.Z && (cc & (u16)CC::G);
  const bool pass_f = _csrs.F && (cc & (u16)CC::F);
  const bool taken = pass_e | pass_l | pass_g | pass_f;
  if (taken) {
    _regs.IP.hi = op.displacement.hi;
    _regs.IP.lo = (_regs.IP.lo + op.displacement.lo) & 0xFFFE;
  }
}

void RegisterBlaster::execute_setmem(tvm::DecodedOp::SetMem op) {
  using StopCause = tvm::StopCause;
  // Not in register mode or there is no system. Either way, comparsion will fail.
  if (_csrs.TR == 1) return hard_stop(StopCause::WrongTR);
  else if (_system == nullptr) return hard_stop(StopCause::MissingSystem);

  // Attempt to convert our ID to a target;
  auto dev = _system->find_by_id(op.target);
  if (!dev) return hard_stop(StopCause::TargetInvalid);
  auto target = dev->capability<Target>();
  if (!target) return hard_stop(StopCause::TargetNotMemory);

  auto dbuff = _mgr->find((pepp::bts::Buffer::ID)op.data.hi);
  // Prevent access to invalid dbuff / past its end
  if (!dbuff) return hard_stop(StopCause::InvalidDBuffer);
  else if ((size_t)op.data.lo + op.size > dbuff->span().size()) return hard_stop(StopCause::InvalidDBuffer);

  bits::span<const u8> data = dbuff->span().subspan(op.data.lo, op.size);

  // The read-xor-write is one logical access: if the read fails there is nothing meaningful to write back.
  const bool ok = try_access([&] {
    bits::span<const u8> payload = data;
    // If xor-encoded, perform extract data into temporary buffer and ^ our data into that temp
    if (op.xor_encoded) {
      if (_tmp.size() < op.size) _tmp.resize(op.size);
      bits::span<u8> tmp(_tmp.data(), op.size);
      // Lie about access type for this access to avoid side effects.
      target->read(op.offset, tmp, rw_cmp);
      bits::inplace_xor(tmp, data);
      // "swap" temp buffer into data
      payload = tmp;
    }
    target->write(op.offset, payload, op.access);
  });
  _csrs.F = ok ? 0 : 1;
}

void RegisterBlaster::execute_cmpmem(tvm::DecodedOp::CmpMem op) {
  using StopCause = tvm::StopCause;
  // Not in register mode or there is no system. Either way, comparsion will fail.
  if (_csrs.TR == 1) return hard_stop(StopCause::WrongTR);
  else if (_system == nullptr) return hard_stop(StopCause::MissingSystem);

  // Attempt to convert our ID to a target;
  auto dev = _system->find_by_id(op.target);
  if (!dev) return hard_stop(StopCause::TargetInvalid);
  auto target = dev->capability<Target>();
  if (!target) return hard_stop(StopCause::TargetNotMemory);

  if (_tmp.size() < op.size) _tmp.resize(op.size);
  bits::span<u8> actual(_tmp.data(), op.size);
  auto dbuff = _mgr->find((pepp::bts::Buffer::ID)op.data.hi);
  // Prevent access to invalid dbuff / past its end
  if (!dbuff) return hard_stop(StopCause::InvalidDBuffer);
  else if ((size_t)op.data.lo + op.size > dbuff->span().size()) return hard_stop(StopCause::InvalidDBuffer);
  auto expected = dbuff->span().subspan(op.data.lo, op.size);
  if (!try_access([&] { target->read(op.offset, actual, rw_cmp); })) {
    // Z/N are left alone: a read that never happened has nothing to say about ordering, and overwriting them would
    // make a failed compare indistinguishable from a successful "not equal".
    _csrs.F = 1;
    return;
  }
  _csrs.F = 0;
  auto cmp = std::memcmp(actual.data(), expected.data(), op.size);
  // Set conditions according to memcmp result.
  if (cmp == 0) _csrs.Z = 1, _csrs.N = 0;
  else if (cmp < 0) _csrs.Z = 0, _csrs.N = 1;
  else _csrs.Z = 0, _csrs.N = 0;
}

void RegisterBlaster::execute_clrmem(tvm::DecodedOp::ClrMem op) {
  using StopCause = tvm::StopCause;
  // Not in register mode or there is no system. Either way, comparsion will fail.
  if (_csrs.TR == 1) return hard_stop(StopCause::WrongTR);
  else if (_system == nullptr) return hard_stop(StopCause::MissingSystem);

  // Attempt to convert our ID to a target;
  auto dev = _system->find_by_id(op.target);
  if (!dev) return hard_stop(StopCause::TargetInvalid);
  auto target = dev->capability<Target>();
  if (!target) return hard_stop(StopCause::TargetNotMemory);

  _csrs.F = try_access([&] { target->clear(op.data); }) ? 0 : 1;
}

void RegisterBlaster::execute_setreg(tvm::DecodedOp::SetReg op) {
  throw std::runtime_error("SETREG execution not implemented");
}

void RegisterBlaster::execute_cmpreg(tvm::DecodedOp::CmpReg op) {
  using StopCause = tvm::StopCause;
  using R = RegisterScan;
  // Not in register mode or there is no system. Either way, comparsion will fail.
  if (_csrs.TR == 0) return hard_stop(StopCause::WrongTR);
  else if (_scan == nullptr) return hard_stop(StopCause::MissingSystem);

  // Attempt to convert our ID registers
  auto pair = _scan->resolve(op.reg);
  if (pair.first == nullptr) return hard_stop(StopCause::RegisterInvalid);
  // Manually unpack to make debugging easier.
  auto reg = pair.first;
  auto field = pair.second;

  // Prevent access to invalid dbuff / past its end
  auto dbuff = _mgr->find((pepp::bts::Buffer::ID)op.data.hi);
  if (!dbuff) return hard_stop(StopCause::InvalidDBuffer);
  else if ((size_t)op.data.lo + op.size > dbuff->span().size()) return hard_stop(StopCause::InvalidDBuffer);

  // Whole-register comparison.
  if (field == nullptr) {
    // If size mismatch, then we would have to do a partial comparison.
    // That sounds annoying, so skip.
    if (reg->byte_width != op.size) return hard_stop(StopCause::RegisterSizeMismatch);

    // Reading a register goes through its backing target, so it fails the same way a memory access does.
    // Compare unsigned at the register's own width; a failed read leaves Z/N untouched, as in CMPMEM.
    u64 actual = 0, expected = 0;
    const bool ok = try_access([&] {
      switch (reg->byte_width) {
      case 1: actual = _scan->read<u8>(op.reg); break;
      case 2: actual = _scan->read<u16>(op.reg); break;
      case 4: actual = _scan->read<u32>(op.reg); break;
      default: break;
      }
    });
    const auto dat = (pepp::bts::Buffer::ID)op.data.hi;
    switch (reg->byte_width) {
    case 1: expected = read16(dat, op.data.lo) & 0xff; break;
    case 2: expected = read16(dat, op.data.lo); break;
    case 4: expected = ((u32)read16(dat, op.data.lo + 2) << 16) | read16(dat, op.data.lo); break;
    default: return hard_stop(StopCause::RegisterWidthIllegal);
    }
    if (!ok) {
      _csrs.F = 1;
      return;
    }
    _csrs.F = 0;
    if (actual == expected) _csrs.Z = 1, _csrs.N = 0;
    else if (actual < expected) _csrs.Z = 0, _csrs.N = 1;
    else _csrs.Z = 0, _csrs.N = 0;
  } else { // Compare only a single field.
    throw std::runtime_error("Field comparison not implemented");
  }
}

void RegisterBlaster::execute_clrreg(tvm::DecodedOp::ClrReg op) {
  throw std::runtime_error("CLRREG execution not implemented");
}

void RegisterBlaster::execute_traddr(tvm::DecodedOp::TRADDR op) {
  throw std::runtime_error("TRADDR execution not implemented");
}

void RegisterBlaster::execute_ldp(tvm::DecodedOp::LDP) {
  // No-op, since this is just a stupid register copy.
}

void RegisterBlaster::execute_dpincr(tvm::DecodedOp::DPIncr op) {
  _regs.DS = op.DS;

  // When no tracebuffer is available, just increment DP.lo and hope that wrapping around is good enough
  if (_tb == nullptr) {
    _regs.DP.lo += op.dp_incr;
    return;
  }

  // When we have a trace buffer, we can look up the successor/predecessor buffers rather than wrapping around.

  // Use signed 32-bit arithmetic so we can detect both overflow and underflow cleanly.
  int32_t new_lo = static_cast<int32_t>(_regs.DP.lo) + static_cast<int16_t>(op.dp_incr);
  constexpr int32_t BUF_SIZE = static_cast<int32_t>(pepp::bts::Buffer::SIZE);

  if (new_lo >= BUF_SIZE) {
    // Forward overflow: go to successor buffer.
    auto succ = _tb->data_successor(pepp::bts::Buffer::ID{_regs.DP.hi});
    if (succ == pepp::bts::Buffer::ID{0}) return hard_stop(tvm::StopCause::InvalidDBuffer);
    _regs.DP.hi = succ.value;
    _regs.DP.lo = static_cast<u16>(new_lo - BUF_SIZE);
  } else if (new_lo < 0) {
    // Backward underflow: go to predecessor buffer.
    auto pred = _tb->data_predecessor(pepp::bts::Buffer::ID{_regs.DP.hi});
    if (pred == pepp::bts::Buffer::ID{0}) return hard_stop(tvm::StopCause::InvalidDBuffer);
    _regs.DP.hi = pred.value;
    _regs.DP.lo = static_cast<u16>(new_lo + BUF_SIZE);
  } else {
    _regs.DP.lo = static_cast<u16>(new_lo);
  }
}

u16 RegisterBlaster::read16(pepp::bts::Buffer::ID id, u16 offset) {
  using StopCause = tvm::StopCause;
  auto buf = _mgr->find(id);
  if (!buf) return hard_stop(StopCause::InvalidIBuffer), 0;

  auto _data = buf->data();
  return ((u16)_data[offset + 0]) | (u16)_data[offset + 1] << 8;
}

void RegisterBlaster::push(tvm::SegmentPair v) {
  using StopCause = tvm::StopCause;
  if (_regs.SP + 4 > _stack.size()) return soft_stop(StopCause::StackOverflow);
  _stack[_regs.SP + 0] = (u8)(v.hi >> 8);
  _stack[_regs.SP + 1] = (u8)(v.hi & 0xFF);
  _stack[_regs.SP + 2] = (u8)(v.lo >> 8);
  _stack[_regs.SP + 3] = (u8)(v.lo & 0xFF);
  _regs.SP += 4;
}

tvm::SegmentPair RegisterBlaster::pop() {
  // Ensure pop of 2*16-bit registers won't cause underflow.
  if (_regs.SP < 4) return soft_stop(tvm::StopCause::StackUnderflow), tvm::SegmentPair{};

  tvm::SegmentPair v;
  _regs.SP -= 4;
  v.hi = (static_cast<u16>(_stack[_regs.SP + 0]) << 8) | static_cast<u16>(_stack[_regs.SP + 1]);
  v.lo = (static_cast<u16>(_stack[_regs.SP + 2]) << 8) | static_cast<u16>(_stack[_regs.SP + 3]);
  return v;
}