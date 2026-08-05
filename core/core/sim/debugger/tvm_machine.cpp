#include "core/sim/debugger/tvm_machine.hpp"

namespace tvm {

void MachineState::soft_stop(tvm::StopCause cause) {
  csrs.L = 0;
  csrs.F = 0;
  regs.STOP_CAUSE = cause;
}

void MachineState::hard_stop(tvm::StopCause cause) {
  csrs.L = 0;
  csrs.F = 1;
  regs.STOP_CAUSE = cause;
}

void MachineState::restart(RegisterRetention retain) {
  using RR = RegisterRetention;
  switch (retain) {
  case RR::All: break;
  case RR::DP: regs = {.DP = regs.DP, .DS = regs.DS}, csrs = {}; break;
  case RR::None: regs = {}, csrs = {}; break;
  }
  // Hoisted out of the switch so no retention mode depends on Flags{}'s `L : 1 = 1` member initializer to come back
  // live. Every restart is live, with the previous program's failure cleared.
  csrs.L = 1, csrs.F = 0;
}

void MachineState::update_ip(pepp::bts::Buffer::Location loc) { return update_ip(loc.id, loc.offset); }

void MachineState::update_ip(pepp::bts::Buffer::ID id, u16 offset) {
  regs.IP.hi = id.value;
  regs.IP.lo = offset & 0xFFFE;
}

void MachineState::update_dp(pepp::bts::Buffer::Location loc) {
  // No alignment mask, unlike IP: payloads are byte-granular.
  regs.DP.hi = loc.id.value;
  regs.DP.lo = loc.offset;
}

void MachineState::push(tvm::SegmentPair v) {
  if (regs.SP + 4 > _stack.size()) return soft_stop(tvm::StopCause::StackOverflow);
  _stack[regs.SP + 0] = (u8)(v.hi >> 8);
  _stack[regs.SP + 1] = (u8)(v.hi & 0xFF);
  _stack[regs.SP + 2] = (u8)(v.lo >> 8);
  _stack[regs.SP + 3] = (u8)(v.lo & 0xFF);
  regs.SP += 4;
}

tvm::SegmentPair MachineState::pop() {
  // Ensure pop of 2*16-bit registers won't cause underflow.
  if (regs.SP < 4) return soft_stop(tvm::StopCause::StackUnderflow), tvm::SegmentPair{};

  tvm::SegmentPair v;
  regs.SP -= 4;
  v.hi = (static_cast<u16>(_stack[regs.SP + 0]) << 8) | static_cast<u16>(_stack[regs.SP + 1]);
  v.lo = (static_cast<u16>(_stack[regs.SP + 2]) << 8) | static_cast<u16>(_stack[regs.SP + 3]);
  return v;
}

u16 read16(pepp::bts::BufferManager &mgr, MachineState &state, pepp::bts::Buffer::ID id, u16 offset) {
  auto buf = mgr.find(id);
  if (!buf) return state.hard_stop(tvm::StopCause::InvalidIBuffer), 0;

  auto data = buf->span().subspan(offset, 2);
  return ((u16)data[0]) | (u16)data[1] << 8;
}

} // namespace tvm
