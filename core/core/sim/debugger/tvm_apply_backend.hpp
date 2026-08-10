#pragma once
#include <memory>
#include <vector>
#include "core/ds/alloc/pagechain.hpp"
#include "core/sim/debugger/register_scanner.hpp"
#include "core/sim/debugger/tvm_backend.hpp"

// The system class from core/sim/system.hpp
class System;

namespace tvm {

// The backend that actually reprograms the simulated machine: SET* writes targets and registers, CMP* reads them back
// and sets N/Z, CLR* resets them.
//
// Every handler here can fail in two distinguishable ways. A structural problem -- wrong TR mode, no System, an ID that
// resolves to nothing, a data buffer that does not cover the requested bytes -- hard-stops the machine, because
// continuing would be meaningless.
// A *target* access that throws only sets F, because the program is still coherent and a following BRF may well be
// there to handle it.
class ApplyBackend : public Backend {
public:
  // System may be null, in which case all ops touching a system fail with a hard-stop.
  ApplyBackend(std::shared_ptr<pepp::bts::BufferManager> mgr, System *system = nullptr);

  System *system() const { return _system; }

  void on_setmem(MachineState &state, const tvm::DecodedOp::SetMem &op) override;
  void on_cmpmem(MachineState &state, const tvm::DecodedOp::CmpMem &op) override;
  void on_clrmem(MachineState &state, const tvm::DecodedOp::ClrMem &op) override;
  void on_setreg(MachineState &state, const tvm::DecodedOp::SetReg &op) override;
  void on_cmpreg(MachineState &state, const tvm::DecodedOp::CmpReg &op) override;
  void on_clrreg(MachineState &state, const tvm::DecodedOp::ClrReg &op) override;
  void on_traddr(MachineState &state, const tvm::DecodedOp::TRADDR &op) override;

private:
  std::shared_ptr<pepp::bts::BufferManager> _mgr;
  System *_system = nullptr;
  RegisterScan *_scan = nullptr;
  // Scratch for read-xor-write and for compare reads. Grows to the widest access seen and is then reused.
  std::vector<u8> _tmp;
};

} // namespace tvm
