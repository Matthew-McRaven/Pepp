#pragma once
#include "core/sim/debugger/tvm_encoding.hpp"
#include "core/sim/debugger/tvm_machine.hpp"
#include "core/sim/debugger/tvm_opcodes.hpp"

namespace tvm {
class TraceBuffer;

// Which way a trace is being replayed. Not machine state -- no opcode can read or write it, and MachineState::restart
// must not reset it -- so it lives on the Backend as replay policy.
enum class Direction : u8 { Forward, Backward };

enum class AccessMode : u8 {
  // When performing memory access, use the access field recorded in the trace. This is the default.
  AsTraced,
  // Do not use the access field recorded in the trace as written. Replace the Operation type with BufferInternal,
  // keeping all other fields.
  ReplaceWithInternal,
};

// Decoding instruction bytes is common to every backend, but doing something with decoded bytes varies by goal.
// This split exists because I want to do at least two things with the same trace program
//   - the interpreter, which applies the described writes to a System instance, which is used to implement step
//     backwards
//   - an analyzer, one of which reports which target memory locations would be touched w/o touching, which is
//     used to implement dirty tracking of main memory.
//
// An backend refuses an op by implementing that handler with a hard_stop. I did not use an opcode mask,
// because some backends might want to implement a subset of ops which share a DecodedOP (e.g., implement unconditional
// branches only).
class Backend {
public:
  virtual ~Backend() = default;

  // Used by on_dpincr to walk the data chain when DP crosses a buffer boundary. Without one, DP.lo just wraps in the
  // current buffer.
  void set_trace_buffer(tvm::TraceBuffer *tb) { _tb = tb; }
  tvm::TraceBuffer *trace_buffer() const { return _tb; }

  // --- Replay direction ---
  //
  // Encoded as one signed depth so the hot-path query is a sign test. `_floor` is the value at zero INVCALL nesting:
  // 0 when replaying forward, -1 when replaying backward. INVCALL increments and INVRET decrements, so entering an
  // invertible subroutine from a backward replay lands on 0 -- which reads as forward, which is exactly the suspension
  // INVCALL exists to provide. Leaving it returns to -1.
  //
  // The floor is stored rather than inferred because -1 is ambiguous on its own: it means "backward, balanced" or
  // "forward, one INVRET too many", and those must not be confused.
  void set_direction(Direction d) { _depth = _floor = (d == Direction::Backward ? -1 : 0); }
  // The direction as the *currently executing code* sees it, i.e. after any INVCALL suspension.
  bool is_forward() const { return _depth >= 0; }
  // The direction the replay as a whole is running, ignoring suspension.
  Direction direction() const { return _floor < 0 ? Direction::Backward : Direction::Forward; }

  // Allow replacing the recorded access type with BufferInternal, which allows the trace VM to avoid creating
  // additional traces when "undo"ing
  void set_access_mode(AccessMode m) { _access_mode = m; }
  AccessMode access_mode() const { return _access_mode; }

  // Dispatch `decoded` to the matching handler. Not virtual: which handler an alternative belongs to is part of the
  // ISA, not part of what a backend gets to decide.
  void dispatch(MachineState &state, const tvm::DecodedOp::OpChoice &decoded);

  // Ops that only modify trace VM state.
  virtual void on_halt(MachineState &state, const tvm::DecodedOp::Halt &op);
  virtual void on_ret(MachineState &state, const tvm::DecodedOp::Ret &op);
  virtual void on_call(MachineState &state, const tvm::DecodedOp::Call &op);
  virtual void on_invcall(MachineState &state, const tvm::DecodedOp::InvCall &op);
  virtual void on_invret(MachineState &state, const tvm::DecodedOp::InvRet &op);
  virtual void on_asyn(MachineState &state, const tvm::DecodedOp::ASyn &op);
  virtual void on_isyn(MachineState &state, const tvm::DecodedOp::ISyn &op);
  virtual void on_lmr(MachineState &state, const tvm::DecodedOp::LMR &op);
  virtual void on_br(MachineState &state, const tvm::DecodedOp::BR &op);
  virtual void on_ldp(MachineState &state, const tvm::DecodedOp::LDP &op);
  virtual void on_dpincr(MachineState &state, const tvm::DecodedOp::DPIncr &op);

  // Ops that involve the target under test.
  virtual void on_setmem(MachineState &state, const tvm::DecodedOp::SetMem &op) = 0;
  virtual void on_cmpmem(MachineState &state, const tvm::DecodedOp::CmpMem &op) = 0;
  virtual void on_clrmem(MachineState &state, const tvm::DecodedOp::ClrMem &op) = 0;
  virtual void on_setreg(MachineState &state, const tvm::DecodedOp::SetReg &op) = 0;
  virtual void on_cmpreg(MachineState &state, const tvm::DecodedOp::CmpReg &op) = 0;
  virtual void on_clrreg(MachineState &state, const tvm::DecodedOp::ClrReg &op) = 0;
  virtual void on_traddr(MachineState &state, const tvm::DecodedOp::TRADDR &op) = 0;
  virtual void on_mmio(MachineState &state, const tvm::DecodedOp::MMIO &op) = 0;

protected:
  Operation effective_access(const Operation &recorded) const {
    if (_access_mode == AccessMode::AsTraced) return recorded;
    return Operation(Operation::Type::BufferInternal, recorded.kind, recorded.initiator);
  }

  tvm::TraceBuffer *_tb = nullptr;
  AccessMode _access_mode = AccessMode::AsTraced;
  // Count the number of invcalls vs invrets. If negative, direction will be Backwards.
  // Must be signed because we use -1 to represent backwards.
  // Floor is set via set_direction and unchanged from there. Depth is modified on invcall and invret.
  // Halting a program where they are not == causes a hard stop.
  int _depth = 0, _floor = 0;
};

} // namespace tvm
