#include "core/sim/debugger/tvm_decoder.hpp"
#include <algorithm>

namespace tvm {

Decoder::Decoder(std::shared_ptr<pepp::bts::BufferManager> mgr, MachineState &state)
    : _mgr(std::move(mgr)), _state(state) {}

void Decoder::decode() {
  auto &regs = _state.regs;
  // Honour the previous instruction's MODCLR bit. This is a decode-stage concern -- CLRMOD means "clear MOD1/MOD2 at
  // the start of the next instruction", and this is that start -- and keeping it here means Decoder+Backend is a
  // complete pair. Leaving it in the driver would silently hand wrong operands to anyone who drove the two directly,
  // since a retained MOD register is indistinguishable from a supplied one.
  if (_state.csrs.CLRMOD) {
    regs.MOD1 = {};
    regs.MOD2 = {};
    _state.csrs.M1 = 0, _state.csrs.M2 = 0;
  }
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
  case Opcode::INVRET: _decoded = decode_invret(ibp, iop); break;
  case Opcode::ASYN: [[fallthrough]];
  case Opcode::ASYNI: _decoded = decode_asyn(ibp, iop); break;
  case Opcode::ISYN: [[fallthrough]];
  case Opcode::ISYNI: _decoded = decode_isyn(ibp, iop); break;
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
  case Opcode::SETMEMX: [[fallthrough]];
  case Opcode::SETMEMI: [[fallthrough]];
  case Opcode::SETMEMXI: _decoded = decode_setmem(ibp, iop); break;
  case Opcode::SETMEMDX: _decoded = decode_setmemdx(ibp, iop); break;
  case Opcode::STEPMEM: [[fallthrough]];
  case Opcode::STEPMEMI: _decoded = decode_stepmem(ibp, iop); break;
  case Opcode::CMPMEM: [[fallthrough]];
  case Opcode::CMPMEMI: _decoded = decode_cmpmem(ibp, iop); break;
  case Opcode::CLRMEM: _decoded = decode_clrmem(ibp, iop); break;
  case Opcode::SETREG: [[fallthrough]]; // All three register ops share a packet layout; only the Delta differs.
  case Opcode::SETREGX: [[fallthrough]];
  case Opcode::STEPREG: [[fallthrough]];
  case Opcode::SETREGI: [[fallthrough]];
  case Opcode::SETREGXI: [[fallthrough]];
  case Opcode::STEPREGI: _decoded = decode_deltareg(ibp, iop); break;
  case Opcode::CMPREG: [[fallthrough]];
  case Opcode::CMPREGI: _decoded = decode_cmpreg(ibp, iop); break;
  case Opcode::CLRREG: _decoded = decode_clrreg(ibp, iop); break;
  case Opcode::TRADDR: _decoded = decode_traddr(ibp, iop); break;
  case Opcode::LDP: _decoded = decode_ldp(ibp, iop); break;
  case Opcode::ACCDP: _decoded = decode_accdp(ibp, iop); break;
  case Opcode::INCDP: _decoded = decode_incdp(ibp, iop); break;
  case Opcode::MMIO: _decoded = decode_mmio(ibp, iop); break;
  case Opcode::MOVMREG: _decoded = decode_movmem2reg(ibp, iop); break;
  case Opcode::LDSEGM: _decoded = decode_loadsegment(ibp, iop); break;
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
  ret.on_forward = ret.on_backward = _state.regs.IP;
  // Targets interleave lo-first, so a near call (both targets in this buffer) costs 2 words instead of 4.
  switch (_state.regs.IS.word_len) {
  default: [[fallthrough]];
  case 4: ret.on_backward.hi = read(ibp, iop + 6); [[fallthrough]];
  case 3: ret.on_forward.hi = read(ibp, iop + 4); [[fallthrough]];
  case 2: ret.on_backward.lo = read(ibp, iop + 2); [[fallthrough]];
  case 1: ret.on_forward.lo = read(ibp, iop + 0); [[fallthrough]];
  case 0: break;
  }
  // Mirror resolved targets into modifier registers if appropriate.
  if (_state.regs.IS.word_len >= 1) _state.regs.MOD1 = ret.on_forward, _state.csrs.M1 = 1;
  if (_state.regs.IS.word_len >= 2) _state.regs.MOD2 = ret.on_backward, _state.csrs.M2 = 1;
  return ret;
}

tvm::DecodedOp::InvRet Decoder::decode_invret(pepp::bts::Buffer::ID ibp, u16 iop) {
  // No-op for decoding; the direction bookkeeping is entirely an execute-stage concern.
  return {};
}

u64 Decoder::decode_syn_data(pepp::bts::Buffer::ID ibp, u16 iop, u8 &size) {
  auto &regs = _state.regs;
  tvm::SegmentPair data = regs.DP;
  u16 wide = regs.DS;
  const auto op = (tvm::Opcode)regs.IS.ocpode;
  const bool immediate = op == tvm::Opcode::ASYNI || op == tvm::Opcode::ISYNI;
  if (immediate && !decode_immediate(ibp, iop, 0, data, wide)) return 0;
  // DS is shared with the SET*/CMP* ops, so it may well be wider than a timestamp. Ignore the excess.
  // Silently truncate to 8 bytes, since our timestamps are at most u64s.
  size = (u8)std::min<u16>(wide, sizeof(u64));
  if (immediate) regs.MOD1.lo = size;

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
  if (buf == nullptr) return _state.hard_stop(StopCause::InvalidIBuffer), ret;
  // Rather than read incrementally, form a span.
  // Bounds check before forming the span to avoid a buffer overflow/UB.
  const auto whole = buf->span();
  const std::size_t first = (std::size_t)iop + 2, bytes = (std::size_t)count * 2;
  if (first + bytes > whole.size()) return _state.hard_stop(StopCause::InvalidIBuffer), ret;
  ret.data = whole.subspan(first, bytes);
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

bool Decoder::decode_immediate(pepp::bts::Buffer::ID ibp, u16 iop, u8 packet_words, tvm::SegmentPair &data,
                               u16 &size) {
  auto &regs = _state.regs;
  // The size word follows the full packet. Without it, there is no payload to point at.
  if (regs.IS.word_len < packet_words + 1) return _state.hard_stop(StopCause::IllegalOpcode), false;
  const u16 at = iop + 2 * packet_words;
  regs.MOD1.lo = read(ibp, at);
  regs.MOD2.hi = regs.IP.hi;
  regs.MOD2.lo = at + 2;
  _state.csrs.M1 = _state.csrs.M2 = 1;
  data = regs.MOD2;
  size = regs.MOD1.lo;
  return true;
}

tvm::DecodedOp::DeltaMem Decoder::decode_setmem(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::DeltaMem ret;
  auto &regs = _state.regs;
  const auto op = (tvm::Opcode)regs.IS.ocpode;
  ret.kind = (op == tvm::Opcode::SETMEMX || op == tvm::Opcode::SETMEMXI) ? tvm::Delta::Xor : tvm::Delta::Assign;
  _state.csrs.TR = 0; // Enter target mode.
  ret.data = regs.DP;
  ret.size = regs.DS;
  static constexpr u8 packet_words = 4;
  const bool immediate = op == tvm::Opcode::SETMEMI || op == tvm::Opcode::SETMEMXI;
  if (immediate && !decode_immediate(ibp, iop, packet_words, ret.data, ret.size)) return ret;

  switch (immediate ? packet_words : regs.IS.word_len) {
  default: [[fallthrough]];
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

tvm::DecodedOp::DeltaMem Decoder::decode_setmemdx(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::DeltaMem ret;
  auto &regs = _state.regs;
  // No non-XOR form of this opcode exists; see Opcode::SETMEMDX.
  ret.kind = tvm::Delta::Xor;
  _state.csrs.TR = 0; // Enter target mode.
  ret.size = regs.DS;

  switch (regs.IS.word_len) {
  default: [[fallthrough]];
  case 2: regs.ID.lo = read(ibp, iop + 2); [[fallthrough]];
  case 1: regs.ACCESS = read(ibp, iop + 0); [[fallthrough]];
  case 0: break;
  }

  // Offset first, then the payload. Bound both together against the buffer, since a truncated data chain would
  // otherwise be read past twice -- once here for the offset and again in the backend for the payload.
  auto dbuff = _mgr->find((pepp::bts::Buffer::ID)regs.DP.hi);
  if (!dbuff) return _state.hard_stop(StopCause::InvalidDBuffer), ret;
  auto span = dbuff->span();
  if ((size_t)regs.DP.lo + SETMEMDX_ADDRESS_BYTES + ret.size > span.size())
    return _state.hard_stop(StopCause::InvalidDBuffer), ret;
  regs.OFF.hi = (u16)span[regs.DP.lo + 0] | ((u16)span[regs.DP.lo + 1] << 8);
  regs.OFF.lo = (u16)span[regs.DP.lo + 2] | ((u16)span[regs.DP.lo + 3] << 8);

  // DS stays the payload size, so the payload begins past the offset rather than at DP.
  ret.data = tvm::SegmentPair{.hi = regs.DP.hi, .lo = (u16)(regs.DP.lo + SETMEMDX_ADDRESS_BYTES)};
  ret.access = Operation(regs.ACCESS);
  ret.target = (Device::ID)regs.ID.lo;
  ret.offset = regs.OFF.as_u32();

  return ret;
}

tvm::DecodedOp::DeltaMem Decoder::decode_stepmem(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::DeltaMem ret;
  auto &regs = _state.regs;
  ret.kind = tvm::Delta::Add;
  _state.csrs.TR = 0; // Enter target mode.
  ret.data = regs.DP;
  ret.size = regs.DS;
  // If MOD1 is not set, then choose LE by default.
  ret.order = decode_order(_state.csrs.M1 && regs.MOD1.hi);
  static constexpr u8 packet_words = 5;
  const bool immediate = regs.IS.ocpode == (u8)tvm::Opcode::STEPMEMI;
  if (immediate && !decode_immediate(ibp, iop, packet_words, ret.data, ret.size)) return ret;

  switch (immediate ? packet_words : regs.IS.word_len) {
  default: [[fallthrough]];
  case 5:
    regs.MOD1.hi = read(ibp, iop + 8), _state.csrs.M1 = 1;
    ret.order = decode_order(regs.MOD1.hi);
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
  ret.data = regs.DP;
  ret.size = regs.DS;
  static constexpr u8 packet_words = 3;
  const bool immediate = regs.IS.ocpode == (u8)tvm::Opcode::CMPMEMI;
  if (immediate && !decode_immediate(ibp, iop, packet_words, ret.data, ret.size)) return ret;

  switch (immediate ? packet_words : regs.IS.word_len) {
  default: [[fallthrough]];
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

tvm::DecodedOp::DeltaReg Decoder::decode_deltareg(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::DeltaReg ret;
  auto &regs = _state.regs;
  bool immediate = false;
  switch ((tvm::Opcode)regs.IS.ocpode) {
  case tvm::Opcode::SETREGXI: immediate = true; [[fallthrough]];
  case tvm::Opcode::SETREGX: ret.kind = tvm::Delta::Xor; break;
  case tvm::Opcode::STEPREGI: immediate = true; [[fallthrough]];
  case tvm::Opcode::STEPREG: ret.kind = tvm::Delta::Add; break;
  case tvm::Opcode::SETREGI: immediate = true; [[fallthrough]];
  default: ret.kind = tvm::Delta::Assign; break;
  }
  _state.csrs.TR = 1; // Enter register mode.
  ret.data = regs.DP;
  ret.size = regs.DS;
  static constexpr u8 packet_words = 3;
  if (immediate && !decode_immediate(ibp, iop, packet_words, ret.data, ret.size)) return ret;

  switch (immediate ? packet_words : regs.IS.word_len) {
  default: [[fallthrough]];
  case 3: regs.ID.lo = read(ibp, iop + 4); [[fallthrough]];
  case 2: regs.ID.hi = read(ibp, iop + 2); [[fallthrough]];
  case 1: regs.ACCESS = read(ibp, iop + 0); [[fallthrough]];
  case 0: break;
  }
  // regs.ACCESS is still programmed above for whatever later instruction retains it; it just has no bearing on a
  // register op, which reaches its device through RegisterScan rather than through a Target of its own.
  ret.reg =
      RegisterScan::RegisterRef{RegisterScan::Register::ID{regs.ID.hi}, RegisterScan::Register::Field::ID{regs.ID.lo}};
  return ret;
}

tvm::DecodedOp::CmpReg Decoder::decode_cmpreg(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::CmpReg ret;
  auto &regs = _state.regs;
  _state.csrs.TR = 1; // Enter register mode.
  ret.data = regs.DP;
  ret.size = regs.DS;
  static constexpr u8 packet_words = 2;
  const bool immediate = regs.IS.ocpode == (u8)tvm::Opcode::CMPREGI;
  if (immediate && !decode_immediate(ibp, iop, packet_words, ret.data, ret.size)) return ret;

  switch (immediate ? packet_words : regs.IS.word_len) {
  default: [[fallthrough]];
  case 2: regs.ID.lo = read(ibp, iop + 2); [[fallthrough]];
  case 1: regs.ID.hi = read(ibp, iop + 0); [[fallthrough]];
  case 0: break;
  }
  ret.reg =
      RegisterScan::RegisterRef{RegisterScan::Register::ID{regs.ID.hi}, RegisterScan::Register::Field::ID{regs.ID.lo}};
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
  ret.reg =
      RegisterScan::RegisterRef{RegisterScan::Register::ID{regs.ID.hi}, RegisterScan::Register::Field::ID{regs.ID.lo}};
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

DecodedOp::MMIO Decoder::decode_mmio(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::MMIO ret;
  auto &regs = _state.regs;
  _state.csrs.TR = 0; // Enter target mode.
  ret.size = 1;
  ret.write = false;

  switch (regs.IS.word_len) {
  default: [[fallthrough]];
  case 3:
    regs.MOD1.lo = read(ibp, iop + 4), _state.csrs.M1 = 1;
    ret.write = regs.MOD1.lo;
    [[fallthrough]];
  case 2: regs.ID.lo = read(ibp, iop + 2); [[fallthrough]];
  case 1: regs.ACCESS = read(ibp, iop + 0); [[fallthrough]];
  case 0: break;
  }

  // Offset first, then the payload. Bound both together against the buffer.
  auto dbuff = _mgr->find((pepp::bts::Buffer::ID)regs.DP.hi);
  if (!dbuff) return _state.hard_stop(StopCause::InvalidDBuffer), ret;
  auto span = dbuff->span();
  if ((size_t)regs.DP.lo + MMIO_PROLOGUE_BYTES + ret.size > span.size())
    return _state.hard_stop(StopCause::InvalidDBuffer), ret;
  regs.OFF.hi = (u16)span[regs.DP.lo + 0] | ((u16)span[regs.DP.lo + 1] << 8);
  regs.OFF.lo = (u16)span[regs.DP.lo + 2] | ((u16)span[regs.DP.lo + 3] << 8);

  ret.data = span[regs.DP.lo + MMIO_PROLOGUE_BYTES];

  ret.access = Operation(regs.ACCESS);
  ret.target = (Device::ID)regs.ID.lo;
  ret.offset = regs.OFF.as_u32();

  return ret;
}

DecodedOp::MovMem2Reg Decoder::decode_movmem2reg(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::MovMem2Reg ret;
  auto &regs = _state.regs;
  _state.csrs.TR = 1;

  switch (regs.IS.word_len) {
  default: [[fallthrough]];
  case 7: regs.MOD1.lo = read(ibp, iop + 12); [[fallthrough]];
  case 6: regs.MOD1.hi = read(ibp, iop + 10), _state.csrs.M1 = 1; [[fallthrough]];
  case 5: regs.OFF.lo = read(ibp, iop + 8); [[fallthrough]];
  case 4: regs.OFF.hi = read(ibp, iop + 6); [[fallthrough]];
  case 3: regs.ID.lo = read(ibp, iop + 4); [[fallthrough]];
  case 2: regs.ID.hi = read(ibp, iop + 2); [[fallthrough]];
  case 1: regs.ACCESS = read(ibp, iop + 0); [[fallthrough]];
  case 0: break;
  }

  ret.offset = regs.OFF.as_u32();
  ret.src = Device::ID{(u8)(_state.csrs.M1 ? regs.MOD1.lo : 0)};
  ret.dst =
      RegisterScan::RegisterRef{RegisterScan::Register::ID{regs.ID.hi}, RegisterScan::Register::Field::ID{regs.ID.lo}};
  ret.byteswap = _state.csrs.M1 ? regs.MOD1.hi : 0;
  ret.access = Operation(regs.ACCESS);

  return ret;
}

DecodedOp::LoadSegment Decoder::decode_loadsegment(pepp::bts::Buffer::ID ibp, u16 iop) {
  tvm::DecodedOp::LoadSegment ret;
  auto &regs = _state.regs;
  _state.csrs.TR = 0;

  switch (regs.IS.word_len) {
  default: [[fallthrough]];
  case 4: regs.MOD1.lo = read(ibp, iop + 6); [[fallthrough]];
  case 3: regs.MOD1.hi = read(ibp, iop + 4), _state.csrs.M1 = 1; [[fallthrough]];
  case 2: regs.ID.lo = read(ibp, iop + 2); [[fallthrough]];
  case 1: regs.ACCESS = read(ibp, iop + 0); [[fallthrough]];
  case 0: break;
  }

  ret.kind = (Loadable::MemoryKind)regs.ACCESS;
  ret.dst = Device::ID{(u8)regs.ID.lo};
  if (_state.csrs.M1) ret.src = SegmentHandle((u32(regs.MOD1.hi) << 16) | regs.MOD1.lo);

  return ret;
}

} // namespace tvm
