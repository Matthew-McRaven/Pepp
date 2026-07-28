#include "register_blaster.hpp"
#include <bit>
#include "core/sim/debugger/register_scanner.hpp"
#include "core/sim/system.hpp"

RegisterBlaster::RegisterBlaster(std::shared_ptr<pepp::bts::BufferManager> mgr, System *system)
    : _mgr(mgr), _system(system) {
  if (_system) _scan = _system->register_scan();
  else _scan = nullptr;
}

void RegisterBlaster::update_ip(pepp::bts::BufferLocation loc) { return update_ip(loc.id, loc.offset); }

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
  execute();
}

void RegisterBlaster::run_direct(pepp::bts::BufferLocation loc) {
  // Bring the blaster back into the live state (L==1).
  // Clear F bit in case last program terminated with hardfail.
  // "soft stop" is L==0,F==0, "hard stop is L==0,F==1.
  _csrs.L = 1, _csrs.F = 0;
  update_ip(loc);
  while (_csrs.L) step();
}

void RegisterBlaster::run_indirect(std::span<pepp::bts::BufferLocation> locs) {
  // Run the program at each location. Check for a hard stop condition. On hard stop, abort the loop.
  // On a normal/soft stop, resume execution of the next program.
  for (const auto &loc : locs)
    if (run_direct(loc); _csrs.F == 1) break;
}

pepp::bts::Buffer *RegisterBlaster::ibuffer() { return _mgr->find((pepp::bts::Buffer::ID)_regs.IP.hi); }

void RegisterBlaster::set_soft_stop() {
  _csrs.L = 0;
  _csrs.F = 0;
}

void RegisterBlaster::set_hard_stop() {
  _csrs.L = 0;
  _csrs.F = 1;
}

void RegisterBlaster::decode() {
  const auto ibp = (pepp::bts::Buffer::ID)_regs.IP.hi;
  // read 16 bits at ip.lo from data and increment.
  u16 opcode = read16(ibp, _regs.IP.lo);
  // Perform bit-cracking to expose fields
  _regs.IS = OpWord(opcode);
  _csrs.CLRMOD = _regs.IS.clrmod;
  // Whenever a packet needs an a refernece relative to the IP's offset into the current buffer,
  // refer to this variable that than IP. This lets us pre-increment IP and avoid difficulties with branching / early
  // returns.
  const auto iop = 2 + _regs.IP.lo;

  // Mask out low-order bit, because opcodes are naturally aligned.
  _regs.IP.lo = (_regs.IP.lo + 2 + _regs.IS.word_len * 2) & 0xFFFE;
  switch (static_cast<Opcode>(_regs.IS.ocpode)) {
  case Opcode::HALT: {
    switch (_regs.IS.word_len) {
    default: [[fallthrough]];
    case 1: _regs.MOD1.lo = read16(ibp, iop + 2), _csrs.M1 = 1; [[fallthrough]];
    case 0: break;
    }
    set_soft_stop();
  } break;
  case Opcode::RET: _regs.IP = pop(); break;
  case Opcode::CALL: {
    push(_regs.IP);
    switch (_regs.IS.word_len) {
    default: [[fallthrough]];
    case 2: _regs.IP.hi = read16(ibp, iop + 2); [[fallthrough]];
    case 1: _regs.IP.lo = read16(ibp, iop + 0);
    case 0: break;
    }
  } break;
  case Opcode::SYN: {
    switch (_regs.IS.word_len) {
    default: [[fallthrough]];
    case 4: _regs.MOD2.lo = read16(ibp, iop + 6); [[fallthrough]];
    case 3: _regs.MOD2.hi = read16(ibp, iop + 4), _csrs.M2 = 1; [[fallthrough]];
    case 2: _regs.MOD1.lo = read16(ibp, iop + 2); [[fallthrough]];
    case 1: _regs.MOD1.hi = read16(ibp, iop + 0), _csrs.M1 = 1; [[fallthrough]];
    case 0: break;
    }
  } break;
  case Opcode::LMR: {
    if (_regs.IS.word_len == 0) break;
    u16 mask = read16(ibp, iop + 0);
    u16 pos = 1;
    while (mask != 0 && pos <= _regs.IS.word_len) {
      // Position of lowest set bit.
      int i = std::countr_zero(mask); // position of lowest set bit, 0..13
      // Mask for that single bit.
      RegMask masked = (RegMask)(1u << i);
      mask &= mask - 1; // Clear lowest bit

      u16 temp = read16(ibp, iop + pos * 2);
      pos += 1;

      switch (masked) {
      case RegMask::IP_HI: _regs.IP.hi = temp; break;
      case RegMask::IP_LO:
        _regs.IP.lo = temp & 0xFFFE;
        break; // Must mask low order bit to force IP to be word-aligned.
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
  } break;
  case Opcode::BRF: {
    // Per opcode, set condition code F.
    _regs.MOD1.lo = (u16)ConditionCode::F, _csrs.M1 = 1;
    switch (_regs.IS.word_len) {
    default: [[fallthrough]];
    case 2:
      _regs.MOD2.hi = read16(ibp, iop + 2);
      _regs.MOD2.lo = read16(ibp, iop + 0);
      _csrs.M2 = 1;
      break;
    case 1:
      _regs.MOD2.hi = _regs.IP.hi;
      _regs.MOD2.lo = read16(ibp, iop + 0);
      _csrs.M2 = 1;
      break;
    case 0: break;
    }
  } break;
  case Opcode::NOP: [[fallthrough]];
  case Opcode::BREQ: [[fallthrough]];
  case Opcode::BRGT: [[fallthrough]];
  case Opcode::BRGE: [[fallthrough]];
  case Opcode::BRLT: [[fallthrough]];
  case Opcode::BRLE: [[fallthrough]];
  case Opcode::BRNE: [[fallthrough]];
  case Opcode::BR: {
    // Per Opcode, lge bits are the lowest 3 bits, so mask is 0x7.
    _regs.MOD1.lo = _regs.IS.ocpode & 0x7, _csrs.M1 = 1;
    switch (_regs.IS.word_len) {
    default: [[fallthrough]];
    case 2:
      _regs.MOD2.hi = read16(ibp, iop + 2);
      _regs.MOD2.lo = read16(ibp, iop + 0);
      _csrs.M2 = 1;
      break;
    case 1:
      _regs.MOD2.hi = _regs.IP.hi;
      _regs.MOD2.lo = read16(ibp, iop + 0);
      _csrs.M2 = 1;
      break;
    case 0: break;
    }
  } break;
  case Opcode::SETMEM: [[fallthrough]]; // Difference between SETMEM/X is in execution, not decoding
  case Opcode::SETMEMX: {
    switch (_regs.IS.word_len) {
    default: [[fallthrough]];
    case 5:
      _regs.DS = read16(ibp, iop + 8);
      _regs.DP.hi = _regs.IP.hi;
      _regs.DP.lo = iop + 10;
      [[fallthrough]];
    case 4: _regs.OFF.lo = read16(ibp, iop + 6); [[fallthrough]];
    case 3: _regs.OFF.hi = read16(ibp, iop + 4); [[fallthrough]];
    case 2: _regs.ID.lo = read16(ibp, iop + 2), _csrs.TR = 0; [[fallthrough]];
    case 1: _regs.ACCESS = read16(ibp, iop + 0); [[fallthrough]];
    case 0: break;
    }
  } break;
  case Opcode::CMPMEM: {
    switch (_regs.IS.word_len) {
    default: [[fallthrough]];
    case 5:
      _regs.DS = read16(ibp, iop + 8);
      _regs.DP.hi = _regs.IP.hi;
      _regs.DP.lo = iop + 10;
      [[fallthrough]];
    case 4: _regs.OFF.lo = read16(ibp, iop + 6); [[fallthrough]];
    case 3: _regs.OFF.hi = read16(ibp, iop + 4); [[fallthrough]];
    case 2: _regs.ID.lo = read16(ibp, iop + 2), _csrs.TR = 0; [[fallthrough]];
    case 1: _regs.MOD1.lo = read16(ibp, iop + 0), _csrs.M1 = 1; [[fallthrough]];
    case 0: break;
    }
  } break;
  case Opcode::CLRMEM: {
    switch (_regs.IS.word_len) {
    default: [[fallthrough]];
    case 2: _regs.ID.lo = read16(ibp, iop + 2), _csrs.TR = 0; [[fallthrough]];
    case 1: _regs.MOD1.lo = read16(ibp, iop + 0), _csrs.M1 = 1; [[fallthrough]];
    case 0: break;
    }
  } break;
  case Opcode::SETREG: [[fallthrough]]; // Difference between SETREG/X is in execution, not decoding
  case Opcode::SETREGX: {
    switch (_regs.IS.word_len) {
    default: [[fallthrough]];
    case 5:
      _regs.DS = read16(ibp, iop + 8);
      _regs.DP.hi = _regs.IP.hi;
      _regs.DP.lo = iop + 10;
      [[fallthrough]];
    case 4: _regs.OFF.lo = read16(ibp, iop + 6); [[fallthrough]];
    case 3: _regs.ID.lo = read16(ibp, iop + 4); [[fallthrough]];
    case 2: _regs.ID.hi = read16(ibp, iop + 2), _csrs.TR = 1; [[fallthrough]];
    case 1: _regs.ACCESS = read16(ibp, iop + 0); [[fallthrough]];
    case 0: break;
    }
  } break;
  case Opcode::CMPREG: {
    switch (_regs.IS.word_len) {
    default: [[fallthrough]];
    case 5:
      _regs.DS = read16(ibp, iop + 8);
      _regs.DP.hi = _regs.IP.hi;
      _regs.DP.lo = iop + 10;
      [[fallthrough]];
    case 4: _regs.OFF.lo = read16(ibp, iop + 6); [[fallthrough]];
    case 3: _regs.ID.lo = read16(ibp, iop + 4); [[fallthrough]];
    case 2: _regs.ID.hi = read16(ibp, iop + 2), _csrs.TR = 1; [[fallthrough]];
    case 1: _regs.MOD1.lo = read16(ibp, iop + 0), _csrs.M1 = 1; [[fallthrough]];
    case 0: break;
    }
  } break;
  case Opcode::CLRREG: {
    switch (_regs.IS.word_len) {
    default: [[fallthrough]];
    case 2: _regs.ID.lo = read16(ibp, iop + 2); [[fallthrough]];
    case 1: _regs.ID.hi = read16(ibp, iop + 0), _csrs.TR = 1; [[fallthrough]];
    case 0: break;
    }
  } break;
  case Opcode::TRADDR: {
    switch (_regs.IS.word_len) {
    default: [[fallthrough]];
    case 7: _regs.DS = read16(ibp, iop + 12); [[fallthrough]];
    case 6: _regs.DP.lo = read16(ibp, iop + 10); [[fallthrough]];
    case 5: _regs.DP.hi = read16(ibp, iop + 8); [[fallthrough]];
    case 4: _regs.ID.hi = read16(ibp, iop + 6); [[fallthrough]];
    case 3: _regs.OFF.lo = read16(ibp, iop + 4); [[fallthrough]];
    case 2: _regs.OFF.hi = read16(ibp, iop + 2); [[fallthrough]];
    case 1: _regs.ID.lo = read16(ibp, iop + 0), _csrs.TR = 0; [[fallthrough]];
    case 0: break;
    }
  }
    // Treat unrecognized upcodes as hard failures.
  default: set_hard_stop(); break;
  }
}

void RegisterBlaster::execute() {
  // For instructions which don't just program registers, insert their behaviors here
  switch (static_cast<Opcode>(_regs.IS.ocpode)) {

  case Opcode::BRF: [[fallthrough]];
  case Opcode::NOP: [[fallthrough]];
  case Opcode::BREQ: [[fallthrough]];
  case Opcode::BRGT: [[fallthrough]];
  case Opcode::BRGE: [[fallthrough]];
  case Opcode::BRLT: [[fallthrough]];
  case Opcode::BRLE: [[fallthrough]];
  case Opcode::BRNE: [[fallthrough]];
  case Opcode::BR: {
    using namespace bits;
    using CC = RegisterBlaster::ConditionCode;
    const u16 cc = _regs.MOD1.lo & (u16)CC::MASK;
    const bool pass_e = _csrs.Z && (cc & (u16)CC::E);
    const bool pass_l = _csrs.N && (cc & (u16)CC::L);
    const bool pass_g = !_csrs.N && !_csrs.Z && (cc & (u16)CC::G);
    const bool pass_f = _csrs.F && (cc & (u16)CC::F);
    const bool taken = pass_e | pass_l | pass_g | pass_f;
    if (taken & _csrs.M2) {
      _regs.IP.lo = (_regs.IP.lo + _regs.MOD2.lo) & 0xFFFE;
      _regs.IP.hi = _regs.MOD2.hi;
    }
  }
  // TODO: handle calls into system!
  // case Opcode::SETMEM:
  // case Opcode::SETMEMX:
  // case Opcode::SETREG:
  // case Opcode::SETREGX:
  // case Opcode::CMPMEM:
  // case Opcode::CMPREG:
  // case Opcode::CLRMEM:
  // case Opcode::CLRREG:
  default: break;
  }
}

u16 RegisterBlaster::read16(pepp::bts::Buffer::ID id, u16 offset) {
  auto buf = _mgr->find(id);
  if (!buf) {
    _csrs.L = 0;
    _regs.MOD1.lo = (u16)StopCause::InvalidIBuffer;
    return 0;
  }
  auto _data = buf->data();
  return ((u16)_data[offset + 0]) | (u16)_data[offset + 1] << 8;
}

void RegisterBlaster::push(SegmentPair v) {
  if (_regs.SP + 4 > _stack.size()) {
    _csrs.L = 0;
    _regs.MOD1.lo = (u16)StopCause::StackOverflow;
    return;
  }
  _stack[_regs.SP + 0] = (u8)(v.hi >> 8);
  _stack[_regs.SP + 1] = (u8)(v.hi & 0xFF);
  _stack[_regs.SP + 2] = (u8)(v.lo >> 8);
  _stack[_regs.SP + 3] = (u8)(v.lo & 0xFF);
  _regs.SP += 4;
}

RegisterBlaster::SegmentPair RegisterBlaster::pop() {
  if (_regs.SP < 3) {
    _csrs.L = 0;
    _regs.MOD1.lo = (u16)StopCause::StackUnderflow;
    return {};
  }
  SegmentPair v;
  _regs.SP -= 4;
  v.hi = (static_cast<u16>(_stack[_regs.SP + 0]) << 8) | static_cast<u16>(_stack[_regs.SP + 1]);
  v.lo = (static_cast<u16>(_stack[_regs.SP + 2]) << 8) | static_cast<u16>(_stack[_regs.SP + 3]);
  return v;
}

void init_ibuffer(RegisterBlaster &blaster, pepp::bts::Buffer::ID id, u16 offset) {}
