#pragma once
#include <memory>
#include <stdexcept>
#include <vector>
#include "core/ds/alloc/pagechain.hpp"
#include "core/sim/debugger/register_scanner.hpp"
#include "core/sim/debugger/tvm_backend.hpp"

// The system class from core/sim/system.hpp
class System;

namespace tvm {
class TraceBuffer;

// Run a target access and report if it succeeded, used to catch bad access from targets and set the F bit accordingly.
// A Target refuses an access -- out of range, unmapped, and so on -- by throwing Error, which derives from
// runtime_error. RegisterScan reports the same class of refusal -- not readable, not writable, no such device -- by
// throwing plain std::runtime_error. Without this, a program touching a read-only register would unwind out of the
// interpreter entirely instead of setting F, which is the opposite of how every other refused access behaves.
// RegisterScan ought to grow a typed exception; until it does, this is where the two hierarchies are reconciled.
template <typename Fn> bool try_access(Fn &&fn) {
  try {
    fn();
    return true;
  } catch (const std::runtime_error &) {
    return false;
  }
}

// A backend which allows memory and register access.
// Handlers that fail due to a structural problem (e.g., wrong TR mode, _system==nullptr) will hardstop the machine.
// If a target access fails, the F bit is set, but program execution continues, because the program might be able to
// recover via a BRF.
class ApplyBackend : public Backend {
public:
  // System may be null, in which case all ops touching a system fail with a hard-stop.
  ApplyBackend(std::shared_ptr<pepp::bts::BufferManager> mgr, System *system = nullptr);

  System *system() const { return _system; }

  void on_deltamem(MachineState &state, const tvm::DecodedOp::DeltaMem &op) override;
  void on_cmpmem(MachineState &state, const tvm::DecodedOp::CmpMem &op) override;
  void on_clrmem(MachineState &state, const tvm::DecodedOp::ClrMem &op) override;
  void on_deltareg(MachineState &state, const tvm::DecodedOp::DeltaReg &op) override;
  void on_cmpreg(MachineState &state, const tvm::DecodedOp::CmpReg &op) override;
  void on_clrreg(MachineState &state, const tvm::DecodedOp::ClrReg &op) override;
  void on_traddr(MachineState &state, const tvm::DecodedOp::TRADDR &op) override;
  void on_mmio(MachineState &state, const tvm::DecodedOp::MMIO &op) override;
  void on_movmem2reg(MachineState &state, const tvm::DecodedOp::MovMem2Reg &op) override;
  void on_loadsegment(MachineState &state, const tvm::DecodedOp::LoadSegment &op) override;

protected:
  std::shared_ptr<pepp::bts::BufferManager> _mgr;
  System *_system = nullptr;
  RegisterScan *_scan = nullptr;
  // Scratch for read-xor-write and for compare reads. Grows with widest access and is reused.
  std::vector<u8> _tmp;
};

// A backend intend for replaying traces, whose data chains span more than a single buffer.
class TraceApplyBackend : public ApplyBackend {
public:
  TraceApplyBackend(std::shared_ptr<pepp::bts::BufferManager> mgr, System *system = nullptr,
                    tvm::TraceBuffer *tb = nullptr);

  // Without a buffer, DP crossing a boundary hard-stops, as it does for any other backend.
  void set_trace_buffer(tvm::TraceBuffer *tb) { _tb = tb; }
  tvm::TraceBuffer *trace_buffer() const { return _tb; }

  void on_dpincr(MachineState &state, const tvm::DecodedOp::DPIncr &op) override;
  // Returns to the trace buffer's HALT.
  void on_callhalt(MachineState &state, const tvm::DecodedOp::CallHalt &op) override;
  // Resolves the index to a valid IP value through the trace buffer.
  void on_stcall(MachineState &state, const tvm::DecodedOp::STCALL &op) override;

private:
  tvm::TraceBuffer *_tb = nullptr;
};

} // namespace tvm
