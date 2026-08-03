#include "core/sim/debugger/tvm_backend.hpp"
#include <bit>
#include <variant>
#include "core/sim/debugger/tvm_tracebuffer.hpp"

namespace {
// Visitor for Backend::dispatch. Deliberately a hand-written struct rather than the usual pile of `[&]` lambdas: each
// lambda would be its own closure type carrying its own copy of the two captures, so a 17-alternative overload set
// costs ~272 bytes of stack and 34 stores to build -- per dispatched instruction, and with nothing optimized away in a
// Debug build. This is two pointers, passed in registers.
struct Dispatch {
  tvm::Backend *self;
  tvm::MachineState *state;

  void operator()(const tvm::DecodedOp::Halt &op) const { self->on_halt(*state, op); }
  void operator()(const tvm::DecodedOp::Ret &op) const { self->on_ret(*state, op); }
  void operator()(const tvm::DecodedOp::Call &op) const { self->on_call(*state, op); }
  void operator()(const tvm::DecodedOp::InvCall &op) const { self->on_invcall(*state, op); }
  void operator()(const tvm::DecodedOp::ASyn &op) const { self->on_asyn(*state, op); }
  void operator()(const tvm::DecodedOp::ISyn &op) const { self->on_isyn(*state, op); }
  void operator()(const tvm::DecodedOp::LMR &op) const { self->on_lmr(*state, op); }
  void operator()(const tvm::DecodedOp::BR &op) const { self->on_br(*state, op); }
  void operator()(const tvm::DecodedOp::SetMem &op) const { self->on_setmem(*state, op); }
  void operator()(const tvm::DecodedOp::CmpMem &op) const { self->on_cmpmem(*state, op); }
  void operator()(const tvm::DecodedOp::ClrMem &op) const { self->on_clrmem(*state, op); }
  void operator()(const tvm::DecodedOp::SetReg &op) const { self->on_setreg(*state, op); }
  void operator()(const tvm::DecodedOp::CmpReg &op) const { self->on_cmpreg(*state, op); }
  void operator()(const tvm::DecodedOp::ClrReg &op) const { self->on_clrreg(*state, op); }
  void operator()(const tvm::DecodedOp::TRADDR &op) const { self->on_traddr(*state, op); }
  void operator()(const tvm::DecodedOp::LDP &op) const { self->on_ldp(*state, op); }
  void operator()(const tvm::DecodedOp::DPIncr &op) const { self->on_dpincr(*state, op); }
};
} // namespace

namespace tvm {

void Backend::dispatch(MachineState &state, const tvm::DecodedOp::OpChoice &decoded) {
  // Dispatch on the variant's own discriminant rather than re-deriving it from regs.IS. Decode already committed to an
  // alternative when it built `decoded`, so consulting the opcode a second time would duplicate the opcode-to-handler
  // mapping and make a std::get mismatch (i.e. a thrown bad_variant_access) representable. Visiting instead makes this
  // total by construction: adding an alternative to OpChoice without an overload above is a compile error.
  //
  // Nothing is lost by dropping the opcode. Every distinction a handler needs already lives in the decoded operand --
  // SETMEM vs SETMEMX is `xor_encoded`, the nine branch mnemonics are `condition`, ACCDP vs INCDP is `dp_incr`.
  std::visit(Dispatch{this, &state}, decoded);
}

void Backend::on_halt(MachineState &state, const tvm::DecodedOp::Halt &op) { state.soft_stop(op.cause); }

void Backend::on_ret(MachineState &state, const tvm::DecodedOp::Ret &) { state.regs.IP = state.pop(); }

void Backend::on_call(MachineState &state, const tvm::DecodedOp::Call &op) {
  state.push(state.regs.IP);
  state.regs.IP = op.next_ip;
}

void Backend::on_invcall(MachineState &state, const tvm::DecodedOp::InvCall &op) {
  state.push(state.regs.IP);
  // F is deliberately left as-is: the callee is the one that wants to know whether it was reached because of a
  // failure, and clearing it here would hide that from a subsequent BRF.
  state.regs.IP = state.csrs.F ? op.on_true : op.on_false;
}

// Both sync ops are no-ops for the blaster itself, which keeps no clock. All the work happened in decode, and the
// timestamp is carried in the decoded op for inspection code sitting between the decode and execute stages.
void Backend::on_asyn(MachineState &, const tvm::DecodedOp::ASyn &) {}

void Backend::on_isyn(MachineState &, const tvm::DecodedOp::ISyn &) {}

void Backend::on_lmr(MachineState &state, const tvm::DecodedOp::LMR &op) {
  auto &regs = state.regs;
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
    case RegMask::IP_HI: regs.IP.hi = temp; break;
    case RegMask::IP_LO: regs.IP.lo = temp & 0xFFFE; break; // Must mask low order bit to force IP to be word-aligned.
    case RegMask::DP_HI: regs.DP.hi = temp; break;
    case RegMask::DP_LO: regs.DP.lo = temp; break;
    case RegMask::DS: regs.DS = temp; break;
    case RegMask::ACCESS: regs.ACCESS = temp; break;
    case RegMask::ID_HI: regs.ID.hi = temp; break;
    case RegMask::ID_LO: regs.ID.lo = temp; break;
    case RegMask::OFF_HI: regs.OFF.hi = temp; break;
    case RegMask::OFF_LO: regs.OFF.lo = temp; break;
    case RegMask::MOD1_HI: regs.MOD1.hi = temp, state.csrs.M1 = 1; break;
    case RegMask::MOD1_LO: regs.MOD1.lo = temp, state.csrs.M1 = 1; break;
    case RegMask::MOD2_HI: regs.MOD2.hi = temp, state.csrs.M2 = 1; break;
    case RegMask::MOD2_LO: regs.MOD2.lo = temp, state.csrs.M2 = 1; break;
    case RegMask::FLAGS: {
      bool old_l = state.csrs.L;
      state.csrs = Flags(temp & 0xFF);
      state.csrs.L = old_l;
      break;
    }
    default: break; // Ignore unknown bits. This allows future expansion of the mask without breaking old blasters.
    }
  }
}

void Backend::on_br(MachineState &state, const tvm::DecodedOp::BR &op) {
  using CC = tvm::ConditionCode;
  const u16 cc = ((u16)op.condition) & (u16)CC::MASK;
  const bool pass_e = state.csrs.Z && (cc & (u16)CC::E);
  const bool pass_l = state.csrs.N && (cc & (u16)CC::L);
  const bool pass_g = !state.csrs.N && !state.csrs.Z && (cc & (u16)CC::G);
  const bool pass_f = state.csrs.F && (cc & (u16)CC::F);
  const bool taken = pass_e | pass_l | pass_g | pass_f;
  if (taken) {
    state.regs.IP.hi = op.displacement.hi;
    state.regs.IP.lo = (state.regs.IP.lo + op.displacement.lo) & 0xFFFE;
  }
}

void Backend::on_ldp(MachineState &, const tvm::DecodedOp::LDP &) {
  // No-op, since this is just a stupid register copy.
}

void Backend::on_dpincr(MachineState &state, const tvm::DecodedOp::DPIncr &op) {
  auto &regs = state.regs;
  regs.DS = op.DS;

  // When no tracebuffer is available, just increment DP.lo and hope that wrapping around is good enough
  if (_tb == nullptr) {
    regs.DP.lo += op.dp_incr;
    return;
  }

  // When we have a trace buffer, we can look up the successor/predecessor buffers rather than wrapping around.

  // Use signed 32-bit arithmetic so we can detect both overflow and underflow cleanly.
  int32_t new_lo = static_cast<int32_t>(regs.DP.lo) + static_cast<int16_t>(op.dp_incr);
  constexpr int32_t BUF_SIZE = static_cast<int32_t>(pepp::bts::Buffer::SIZE);

  if (new_lo >= BUF_SIZE) {
    // Forward overflow: go to successor buffer.
    auto succ = _tb->data_successor(pepp::bts::Buffer::ID{regs.DP.hi});
    if (succ == pepp::bts::Buffer::ID{0}) return state.hard_stop(tvm::StopCause::InvalidDBuffer);
    regs.DP.hi = succ.value;
    regs.DP.lo = static_cast<u16>(new_lo - BUF_SIZE);
  } else if (new_lo < 0) {
    // Backward underflow: go to predecessor buffer.
    auto pred = _tb->data_predecessor(pepp::bts::Buffer::ID{regs.DP.hi});
    if (pred == pepp::bts::Buffer::ID{0}) return state.hard_stop(tvm::StopCause::InvalidDBuffer);
    regs.DP.hi = pred.value;
    regs.DP.lo = static_cast<u16>(new_lo + BUF_SIZE);
  } else {
    regs.DP.lo = static_cast<u16>(new_lo);
  }
}

} // namespace tvm
