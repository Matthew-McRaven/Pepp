#pragma once
#include <memory>
#include <span>
#include "core/ds/alloc/pagechain.hpp"
#include "core/integers.h"
#include "core/sim/debugger/tvm_backend.hpp"
#include "core/sim/debugger/tvm_decoder.hpp"
#include "core/sim/debugger/tvm_machine.hpp"

// The system class from core/sim/system.hpp
class System;
namespace tvm {
class TraceBuffer;

// This is basically an ASIC with a custom instruction set used to copy  values into a simulator's
// registers+memory.
// The machine uses little-endian 16-bit words. Opcodes are always one word and called OpWords. Instructions are a
// single 16-bit opcode followed a per-opcode number of 16-bit data words.
// I borrow the concept of a register programming engine that is bytecode-programmable from [AMD's
// Atombios](https://wiki.osdev.org/AMD_Atombios) and the Nova-Core GSP sequencer. Based on an opcode, the blaster
// chooses how to reprogram registers in the target or update internal state.
// Since this blaster will be used for both initial register programming AND tracing, I have extreme pressure to reduce
// memory footprint, even if it introduces dependencies between instructions. The Xilinx 7 series bitstream format
// carries dependencies between its type-1 vs type-2 configuration packets. type-1 packets set initial state (and
// address) which is reused by future type-2 packets. That is, register state is /retained/. Another analogue is JTAG's
// TAP's IR register being retained across data scan registers operations.
// So, what if I retain data in my registers across operations? Well, I'd want to ditch the fixed-sized opcodes of
// Atombios and Nova-Core, since those have decoder-determined instruction lengths. In my design, the length (in u16s)
// is explicitly encoded in the opcode rather than being implicit in the decoder. With a 16-bit opcode I have plenty of
// bits to spare. Because of the variadicity, I reduce the memory footprint for best cases:
// - When an instruction contains less than the number of expected registers, the remaining  registers are retained.
//   With intelligent design-time ordering of operands, common fields can be reused between instructions.
//   For example, short branches can be encoded with a single word rather than 2.
// - When more words are inserted than expected, extra values are ignored.
//   This case is really useful for packing data into an instruction, and is explictly used by SET* instructions.
// In an average case, this reduces exactly to the same number of words as would be used by Atombios or Nova-core,
// except with some extra bookkeeping for the populated word count. My changes over those previous designs
// include:
// - my XOR-encoding for SET*X instructions derives from my previous trace buffer encodings to make a single trace which
//   contains both the "forward" and "backward" data.
// - Explicit call+ret. My work on Pep9Micro taught me that subroutines simplify everything, and I should bake them in
//   from the start.
// - My 3-bit branch encoding, which allows synthesis of all branch types, including a NOP. It  might be the first ISA
//   in history to require a branch prediction for a NOP.
// - Optional registers (MOD1/MOD2) which provide optional data to an instruction that are cleared automatically.
//   Those registers really model a bag-of-properties like my old AST design. The MODCLR bit of an opcode helps clear
//   these automatically.
//
// This class is only a convenience wrapper around other components.
// It owns the MachineState, runs the fetch/decode/execute loop, and applies the RegisterRetention policy between
// programs. Cracking packets belongs to Decoder and acting on them belongs to an Backend, both of which share this
// object's MachineState. Swap the Backend to reuse the ISA for something other than reprogramming a machine --
// inspecting which locations a program would touch, folding deltas together, and so on.
class Interpreter {
public:
  // Retained for callers that spelled these as members of Interpreter before the decode/execute split.
  using RegisterRetention = tvm::RegisterRetention;
  using Flags = tvm::Flags;
  using State = tvm::Registers;

  // Drive `backend`, which must not be null.
  Interpreter(std::shared_ptr<pepp::bts::BufferManager> mgr, std::unique_ptr<Backend> backend);
  // Disable copy/move since this class is EXPENSIVE
  Interpreter(const Interpreter &) = delete;
  Interpreter(Interpreter &&) = delete;
  Interpreter &operator=(const tvm::Interpreter &) = delete;
  Interpreter &operator=(tvm::Interpreter &&) = delete;

  void update_ip(pepp::bts::Buffer::Location loc) { _state.update_ip(loc); }
  void update_ip(pepp::bts::Buffer::ID id, u16 offset = 0) { _state.update_ip(id, offset); }
  // Assuming some code is already under IP, try to run it!
  void step();
  // Update IP to point to loc, then call step() in a loop while L==1.
  // Each program executed this way must terminate with a HALT.
  // At the end of a call to run, L is always 0.
  void run(pepp::bts::Buffer::Location loc, RegisterRetention retain = RegisterRetention::All);
  // For each buffer location set L=1 and call run.
  // Only stops when reaching the end of this location buffer, or on "hard stop", where L==0 && F==1.
  std::size_t run_each(std::span<const pepp::bts::Buffer::Location> locs,
                       RegisterRetention retain = RegisterRetention::DP);
  // Iterator-pair variant. While slower to execute, it can consume iterators from TraceBuffer without needing to
  // re-arrange them in spans first. Declared as a template to avoid include'ing TraceBuffer in this header
  template <typename It> auto run_each(It begin, It end, RegisterRetention retain = RegisterRetention::DP) {
    for (auto it = begin; it != end; ++it) {
      run(*it, retain);
      if (_state.csrs.F == 1) return it;
    }
    return end;
  }
  auto &csrs() { return _state.csrs; }
  const auto &csrs() const { return _state.csrs; }
  auto &regs() { return _state.regs; }
  const auto &regs() const { return _state.regs; }
  // The registers, flags, and stack shared by this driver's decoder and backend.
  MachineState &state() { return _state; }
  const MachineState &state() const { return _state; }
  // The backend this driver dispatches to. Downcast if you need backend-specific results (touched locations, etc).
  Backend &backend() { return *_backend; }
  const Backend &backend() const { return *_backend; }
  pepp::bts::BufferManager &mgr() { return *_mgr; }
  const pepp::bts::BufferManager &mgr() const { return *_mgr; }
  void set_trace_buffer(tvm::TraceBuffer *tb) { _backend->set_trace_buffer(tb); }
  tvm::TraceBuffer *trace_buffer() const { return _backend->trace_buffer(); }
  bool stopped() const { return _state.stopped(); }
  // Why the machine stopped. Distinguish hard/soft stop with F bit. A normal exit uses StopCause::None && F==0.
  tvm::StopCause stop_cause() const { return _state.stop_cause(); }
  // The most recently decoded instruction, with all of its operands already resolved.
  const tvm::DecodedOp::OpChoice &decoded() const { return _decoder.decoded(); }

private:
  std::shared_ptr<pepp::bts::BufferManager> _mgr;
  // Declared before the decoder, which binds a reference to it.
  MachineState _state{};
  Decoder _decoder;
  std::unique_ptr<Backend> _backend;
};
} // namespace tvm
