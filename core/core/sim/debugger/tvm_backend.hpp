#pragma once
#include "core/sim/debugger/tvm_encoding.hpp"
#include "core/sim/debugger/tvm_machine.hpp"
#include "core/sim/debugger/tvm_opcodes.hpp"

namespace tvm {
class TraceBuffer;

// Decoding instruction bytes is common to every backend, but doing something with decoded bytes varies by goal.
// This split exists because I want to do at least two things with the same trace program
//   - the interpreter, which applies the described writes to a System instance, which is used to implement step
//     backwards
//   - an analyzer, one of which reports which target memory locations would be touched w/o touching, which is
//     used to implement dirty tracking of main memory.
//
// Two tiers of handler:
//   - The ops below that only move data around MachineState (control flow, the MOD/DP/register-programming ops) have
//     concrete implementations here, because their meaning is the same everywhere. Override one only to restrict it.
//   - The ops that reach outside the machine (SET*/CMP*/CLR*, TRADDR) are pure virtual, because there is no sensible
//     default for "touch the target".
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

  // Dispatch `decoded` to the matching handler. Not virtual: the opcode-to-alternative mapping is part of the ISA, not
  // part of what a backend gets to decide. Reads the opcode from state.regs.IS, which decode just wrote.
  void dispatch(MachineState &state, const tvm::DecodedOp::OpChoice &decoded);

  // --- Ops that only touch MachineState: shared meaning, concrete here ---
  virtual void on_halt(MachineState &state, const tvm::DecodedOp::Halt &op);
  virtual void on_ret(MachineState &state, const tvm::DecodedOp::Ret &op);
  virtual void on_call(MachineState &state, const tvm::DecodedOp::Call &op);
  virtual void on_invcall(MachineState &state, const tvm::DecodedOp::InvCall &op);
  virtual void on_asyn(MachineState &state, const tvm::DecodedOp::ASyn &op);
  virtual void on_isyn(MachineState &state, const tvm::DecodedOp::ISyn &op);
  virtual void on_lmr(MachineState &state, const tvm::DecodedOp::LMR &op);
  virtual void on_br(MachineState &state, const tvm::DecodedOp::BR &op);
  virtual void on_ldp(MachineState &state, const tvm::DecodedOp::LDP &op);
  virtual void on_dpincr(MachineState &state, const tvm::DecodedOp::DPIncr &op);

  // --- Ops that reach outside the machine: no default is meaningful ---
  virtual void on_setmem(MachineState &state, const tvm::DecodedOp::SetMem &op) = 0;
  virtual void on_cmpmem(MachineState &state, const tvm::DecodedOp::CmpMem &op) = 0;
  virtual void on_clrmem(MachineState &state, const tvm::DecodedOp::ClrMem &op) = 0;
  virtual void on_setreg(MachineState &state, const tvm::DecodedOp::SetReg &op) = 0;
  virtual void on_cmpreg(MachineState &state, const tvm::DecodedOp::CmpReg &op) = 0;
  virtual void on_clrreg(MachineState &state, const tvm::DecodedOp::ClrReg &op) = 0;
  virtual void on_traddr(MachineState &state, const tvm::DecodedOp::TRADDR &op) = 0;

protected:
  tvm::TraceBuffer *_tb = nullptr;
};

} // namespace tvm
