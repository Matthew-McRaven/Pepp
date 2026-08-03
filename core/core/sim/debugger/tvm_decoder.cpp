#include "core/sim/debugger/tvm_decoder.hpp"
#include <algorithm>

namespace tvm {

Decoder::Decoder(std::shared_ptr<pepp::bts::BufferManager> mgr, MachineState &state)
    : _mgr(std::move(mgr)), _state(state) {}

void Decoder::decode() {
  auto &regs = _state.regs;
  const auto ibp = (pepp::bts::Buffer::ID)regs.IP.hi;
  // read 16 bits at ip.lo from data and increment.
  u16 opcode = read(ibp, regs.IP.lo);
  // Perform bit-cracking to expose fields
  regs.IS = tvm::OpWord(opcode);
  _state.csrs.CLRMOD = regs.IS.clrmod;
  // Whenever a packet needs an a refernece relative to the IP's offset into the current buffer,
  // refer to this variable that than IP. This lets us pre-increment IP and avoid difficulties with branching / early
  // returns.
  const auto iop = 2 + regs.IP.lo;

  // Mask out low-order bit, because opcodes are naturally aligned.
  regs.IP.lo = (regs.IP.lo + 2 + regs.IS.word_len * 2) & 0xFFFE;
  switch (static_cast<Opcode>(regs.IS.ocpode)) {
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
  default: _state.hard_stop(StopCause::IllegalOpcode); break; // Treat unrecognized upcodes as hard failures.
  }
}

tvm::DecodedOp::Halt Decoder::decode_halt(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::Halt ret;
  switch (_state.regs.IS.word_len) {
  default: [[fallthrough]];
  case 1: ret.cause = (tvm::StopCause)read(ibp, iop + 0); break;
  case 0: ret.cause = tvm::StopCause::None; break;
  }
  return ret;
}

tvm::DecodedOp::Ret Decoder::decode_ret(pepp::bts::Buffer::ID ibp, u16 iop) {
  // No-op for decoding, since all data is passed on stack.
  return {};
}

tvm::DecodedOp::Call Decoder::decode_call(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::Call ret;
  ret.next_ip.hi = _state.regs.IP.hi, ret.next_ip.lo = iop + 0;
  switch (_state.regs.IS.word_len) {
  default: [[fallthrough]];
  case 2: ret.next_ip.hi = read(ibp, iop + 2); [[fallthrough]];
  case 1: ret.next_ip.lo = read(ibp, iop + 0);
  case 0: break;
  }
  return ret;
}

tvm::DecodedOp::InvCall Decoder::decode_invcall(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::InvCall ret;
  // IP.lo has already been advanced past this packet, so IP is the fall-through address. Any target word the packet
  // omits defaults to it: a missing hi word keeps IP.hi, and a wholly missing target calls the next instruction.
  ret.on_true = ret.on_false = _state.regs.IP;
  // Targets interleave lo-first, so a near call (both targets in this buffer) costs 2 words instead of 4.
  switch (_state.regs.IS.word_len) {
  default: [[fallthrough]];
  case 4: ret.on_false.hi = read(ibp, iop + 6); [[fallthrough]];
  case 3: ret.on_true.hi = read(ibp, iop + 4); [[fallthrough]];
  case 2: ret.on_false.lo = read(ibp, iop + 2); [[fallthrough]];
  case 1: ret.on_true.lo = read(ibp, iop + 0); [[fallthrough]];
  case 0: break;
  }
  // Mirror resolved targets into modifier registers if appropriate.
  if (_state.regs.IS.word_len >= 1) _state.regs.MOD1 = ret.on_true, _state.csrs.M1 = 1;
  if (_state.regs.IS.word_len >= 2) _state.regs.MOD2 = ret.on_false, _state.csrs.M2 = 1;
  return ret;
}

u64 Decoder::decode_syn_data(pepp::bts::Buffer::ID ibp, u16 iop, u8 &size) {
  auto &regs = _state.regs;
  // Unless a size word is provided, data is DP relative rather than immediate.
  tvm::SegmentPair data = regs.DP;
  // DS is shared with the SET*/CMP* ops, so it may well be wider than a timestamp. Ignore the excess.
  size = (u8)std::min<u16>(regs.DS, sizeof(u64));

  if (regs.IS.word_len >= 1) {
    // If MOD1/MOD2 are set, then read from them rather than the data registers.
    // This allows "immediate" versions to avoid clobbering DP regs.
    // Silently truncate to 8 bytes, since our timestamps are at most u64s.
    regs.MOD1.lo = std::min<u16>(read(ibp, iop + 0), sizeof(u64));
    regs.MOD2.hi = regs.IP.hi;
    regs.MOD2.lo = iop + 2;
    _state.csrs.M1 = _state.csrs.M2 = 1;
    data = regs.MOD2;
    size = regs.MOD1.lo;
  }

  u64 value = 0;
  // Ensure that dbuff exists and is in range before reading from it.
  if (auto dbuff = _mgr->find((pepp::bts::Buffer::ID)data.hi); !dbuff)
    return _state.hard_stop(StopCause::InvalidDBuffer), 0;
  else if (auto span = dbuff->span(); (size_t)data.lo + size > span.size())
    return _state.hard_stop(StopCause::InvalidDBuffer), 0;
  else {
    for (u8 i = 0; i < size; ++i) value |= (u64)span[data.lo + i] << (8 * i);
  }

  return value;
}

tvm::DecodedOp::ASyn Decoder::decode_asyn(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::ASyn ret;
  u8 width = 0;
  // Absolute timestamps are unsigned, so a narrow encoding is just the low-order bytes of a bigger number.
  ret.timestamp = decode_syn_data(ibp, iop, width);
  return ret;
}

tvm::DecodedOp::ISyn Decoder::decode_isyn(pepp::bts::Buffer::ID ibp, u16 iop) {
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

tvm::DecodedOp::LMR Decoder::decode_lmr(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::LMR ret;
  if (_state.regs.IS.word_len == 0) return ret;
  ret.mask = (tvm::RegMask)read(ibp, iop + 0);
  auto count = _state.regs.IS.word_len - 1;
  auto buf = _mgr->find(ibp);
  if (buf == nullptr) return ret;
  ret.data = buf->span().subspan(iop + 2, count * 2);
  return ret;
}

tvm::DecodedOp::BR Decoder::decode_br(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::BR ret;
  auto &regs = _state.regs;
  // Select condition code base on the original opcode.
  if (regs.IS.ocpode == (u8)tvm::Opcode::BRF) ret.condition = tvm::ConditionCode::F;
  else ret.condition = (tvm::ConditionCode)(regs.IS.ocpode & 0x7);
  regs.MOD1.lo = (u16)ret.condition, _state.csrs.M1 = 1;

  switch (regs.IS.word_len) {
  default: [[fallthrough]];
  case 2:
    ret.displacement.hi = read(ibp, iop + 2);
    ret.displacement.lo = read(ibp, iop + 0);
    regs.MOD2 = ret.displacement, _state.csrs.M2 = 1;
    break;
  case 1:
    ret.displacement.hi = regs.IP.hi;
    ret.displacement.lo = read(ibp, iop + 0);
    regs.MOD2 = ret.displacement, _state.csrs.M2 = 1;
    break;
  case 0:
    // try to use M2 (if set) as the displacement. If unset, use a 0-displacement from the current op
    if (_state.csrs.M2) ret.displacement = regs.MOD2;
    else ret.displacement = {regs.IP.hi, 0};
    // If MOD2.hi is zero, then treat the displacement as relative to this buffer.
    if (ret.displacement.hi == 0) ret.displacement.hi = regs.IP.hi;
    break;
  }
  return ret;
}

tvm::DecodedOp::SetMem Decoder::decode_setmem(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::SetMem ret;
  auto &regs = _state.regs;
  ret.xor_encoded = (regs.IS.ocpode == (u8)tvm::Opcode::SETMEMX);
  _state.csrs.TR = 0; // Enter target mode.
  // Unless (5) is provided, data is DP relative rather than immediate
  ret.data = regs.DP;
  ret.size = regs.DS;

  switch (regs.IS.word_len) {
  default: [[fallthrough]];
  case 5:
    // If MOD1/MOD2 are set, then read from them rather than the data registers.
    // This allows "immediate" versions to avoid clobbering DP regs.
    regs.MOD1.lo = read(ibp, iop + 8);
    regs.MOD2.hi = regs.IP.hi;
    regs.MOD2.lo = iop + 10;
    _state.csrs.M1 = _state.csrs.M2 = 1;
    ret.data = regs.MOD2;
    ret.size = regs.MOD1.lo;
    [[fallthrough]];
  case 4: regs.OFF.lo = read(ibp, iop + 6); [[fallthrough]];
  case 3: regs.OFF.hi = read(ibp, iop + 4); [[fallthrough]];
  case 2: regs.ID.lo = read(ibp, iop + 2); [[fallthrough]];
  case 1: regs.ACCESS = read(ibp, iop + 0); [[fallthrough]];
  case 0: break;
  }
  ret.access = Operation(regs.ACCESS);
  ret.target = (Device::ID)regs.ID.lo;
  ret.offset = regs.OFF.as_u32();
  return ret;
}

tvm::DecodedOp::CmpMem Decoder::decode_cmpmem(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::CmpMem ret;
  auto &regs = _state.regs;
  _state.csrs.TR = 0; // Enter target mode.
  // Unless (4) is provided, data is DP relative rather than immediate
  ret.data = regs.DP;
  ret.size = regs.DS;

  switch (regs.IS.word_len) {
  default: [[fallthrough]];
  case 4:
    // If MOD1/MOD2 are set, then read from them rather than the data registers.
    // This allows "immediate" versions to avoid clobbering DP regs.
    regs.MOD1.lo = read(ibp, iop + 6);
    regs.MOD2.hi = regs.IP.hi;
    regs.MOD2.lo = iop + 8;
    _state.csrs.M1 = _state.csrs.M2 = 1;
    ret.data = regs.MOD2;
    ret.size = regs.MOD1.lo;
    [[fallthrough]];
  case 3: regs.OFF.lo = read(ibp, iop + 4); [[fallthrough]];
  case 2: regs.OFF.hi = read(ibp, iop + 2); [[fallthrough]];
  case 1: regs.ID.lo = read(ibp, iop + 0); [[fallthrough]];
  case 0: break;
  }
  ret.target = (Device::ID)regs.ID.lo;
  ret.offset = regs.OFF.as_u32();
  return ret;
}

tvm::DecodedOp::ClrMem Decoder::decode_clrmem(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::ClrMem ret;
  auto &regs = _state.regs;
  _state.csrs.TR = 0; // Enter target mode.
  ret.data = 0;

  switch (regs.IS.word_len) {
  default: [[fallthrough]];
  case 2: ret.data = regs.MOD1.lo = read(ibp, iop + 2), _state.csrs.M1 = 1; [[fallthrough]];
  case 1: regs.ID.lo = read(ibp, iop + 0); [[fallthrough]];
  case 0: break;
  }
  ret.target = (Device::ID)regs.ID.lo;
  return ret;
}

tvm::DecodedOp::SetReg Decoder::decode_setreg(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::SetReg ret;
  auto &regs = _state.regs;
  ret.xor_encoded = (regs.IS.ocpode == (u8)tvm::Opcode::SETREGX);
  _state.csrs.TR = 1; // Enter register mode.
  // Unless (4) is provided, data is DP relative rather than immediate
  ret.data = regs.DP;
  ret.size = regs.DS;

  switch (regs.IS.word_len) {
  default: [[fallthrough]];
  case 4:
    // If MOD1/MOD2 are set, then read from them rather than the data registers.
    // This allows "immediate" versions to avoid clobbering DP regs.
    regs.MOD1.lo = read(ibp, iop + 6);
    regs.MOD2.hi = regs.IP.hi;
    regs.MOD2.lo = iop + 8;
    _state.csrs.M1 = _state.csrs.M2 = 1;
    ret.data = regs.MOD2;
    ret.size = regs.MOD1.lo;
    [[fallthrough]];
  case 3: regs.ID.lo = read(ibp, iop + 4); [[fallthrough]];
  case 2: regs.ID.hi = read(ibp, iop + 2); [[fallthrough]];
  case 1: regs.ACCESS = read(ibp, iop + 0); [[fallthrough]];
  case 0: break;
  }
  ret.access = Operation(regs.ACCESS);
  ret.reg = RegisterScan::RegisterRef{RegisterScan::Register::ID{regs.ID.hi},
                                      RegisterScan::Register::Field::ID{regs.ID.lo}};
  return ret;
}

tvm::DecodedOp::CmpReg Decoder::decode_cmpreg(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::CmpReg ret;
  auto &regs = _state.regs;
  _state.csrs.TR = 1; // Enter register mode.
  // Unless (3) is provided, data is DP relative rather than immediate
  ret.data = regs.DP;
  ret.size = regs.DS;

  switch (regs.IS.word_len) {
  default: [[fallthrough]];
  case 3:
    // If MOD1/MOD2 are set, then read from them rather than the data registers.
    // This allows "immediate" versions to avoid clobbering DP regs.
    regs.MOD1.lo = read(ibp, iop + 4);
    regs.MOD2.hi = regs.IP.hi;
    regs.MOD2.lo = iop + 6;
    _state.csrs.M1 = _state.csrs.M2 = 1;
    ret.data = regs.MOD2;
    ret.size = regs.MOD1.lo;
    [[fallthrough]];
  case 2: regs.ID.lo = read(ibp, iop + 2); [[fallthrough]];
  case 1: regs.ID.hi = read(ibp, iop + 0); [[fallthrough]];
  case 0: break;
  }
  ret.reg = RegisterScan::RegisterRef{RegisterScan::Register::ID{regs.ID.hi},
                                      RegisterScan::Register::Field::ID{regs.ID.lo}};
  return ret;
}

tvm::DecodedOp::ClrReg Decoder::decode_clrreg(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::ClrReg ret;
  auto &regs = _state.regs;
  _state.csrs.TR = 1; // Enter register mode.
  switch (regs.IS.word_len) {
  default: [[fallthrough]];
  case 2: regs.ID.lo = read(ibp, iop + 2); [[fallthrough]];
  case 1: regs.ID.hi = read(ibp, iop + 0); [[fallthrough]];
  case 0: break;
  }
  ret.reg = RegisterScan::RegisterRef{RegisterScan::Register::ID{regs.ID.hi},
                                      RegisterScan::Register::Field::ID{regs.ID.lo}};
  return ret;
}

tvm::DecodedOp::TRADDR Decoder::decode_traddr(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::TRADDR ret;
  auto &regs = _state.regs;
  _state.csrs.TR = 0; // Enter target mode

  switch (regs.IS.word_len) {
  default: [[fallthrough]];
  case 8: regs.MOD1.hi = read(ibp, iop + 14); [[fallthrough]];
  case 7: regs.MOD1.lo = read(ibp, iop + 12), _state.csrs.M1 = 1; [[fallthrough]];
  case 6: regs.MOD2.lo = read(ibp, iop + 10); [[fallthrough]];
  case 5: regs.MOD2.hi = read(ibp, iop + 8), _state.csrs.M2 = 1; [[fallthrough]];
  case 4: regs.ID.hi = read(ibp, iop + 6); [[fallthrough]];
  case 3: regs.OFF.lo = read(ibp, iop + 4); [[fallthrough]];
  case 2: regs.OFF.hi = read(ibp, iop + 2); [[fallthrough]];
  case 1: regs.ID.lo = read(ibp, iop + 0); [[fallthrough]];
  case 0: break;
  }

  ret.target = (Device::ID)regs.ID.lo;
  ret.target_offset = regs.OFF.as_u32();
  ret.source = (Device::ID)regs.ID.hi;
  ret.source_offset = regs.MOD2.as_u32();
  ret.size = regs.MOD1.as_u32();
  return ret;
}

tvm::DecodedOp::LDP Decoder::decode_ldp(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::LDP ret;
  auto &regs = _state.regs;
  switch (regs.IS.word_len) {
  default: [[fallthrough]];
  case 3: regs.DP.hi = read(ibp, iop + 4); [[fallthrough]];
  case 2: regs.DS = read(ibp, iop + 2); [[fallthrough]];
  case 1: regs.DP.lo = read(ibp, iop + 0); [[fallthrough]];
  case 0: break;
  }
  return ret;
}

tvm::DecodedOp::DPIncr Decoder::decode_accdp(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::DPIncr ret;
  ret.DS = ret.dp_incr = _state.regs.DS;
  switch (_state.regs.IS.word_len) {
  default: [[fallthrough]];
  case 1: ret.DS = read(ibp, iop + 0);
  case 0: break;
  }
  return ret;
}

tvm::DecodedOp::DPIncr Decoder::decode_incdp(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::DPIncr ret;
  ret.DS = _state.regs.DS;
  ret.dp_incr = 0;
  switch (_state.regs.IS.word_len) {
  default: [[fallthrough]];
  case 2: ret.DS = read(ibp, iop + 2);
  case 1: ret.dp_incr = read(ibp, iop + 0);
  case 0: break;
  }
  return ret;
}

} // namespace tvm
