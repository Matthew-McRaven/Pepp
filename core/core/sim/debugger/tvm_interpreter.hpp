#pragma once
#include <array>
#include <functional>
#include <memory>
#include "core/ds/alloc/pagechain.hpp"
#include "core/integers.h"
#include "core/sim/api/device.hpp"
#include "core/sim/debugger/register_scanner.hpp"
#include "core/sim/debugger/tvm_encoding.hpp"
#include "core/sim/debugger/tvm_opcodes.hpp"

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
// except with some extra bookkeeping for the populated word count. My contributions over those previous designs
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
// While opcode decoding and register programming is handled by this class, the "implementation" of each opcode is
// customizable by providing a callback per-opcode.
// We provide a helper to install the same handler for all BR mnemonics for your convenience.
class Interpreter {
public:
  struct Flags {
    Flags() = default;
    Flags(u8 v)
        : N(v & 0x01), Z((v >> 1) & 0x01), TR((v >> 2) & 0x01), L((v >> 3) & 0x01), M1((v >> 4) & 0x01),
          M2((v >> 5) & 0x01), CLRMOD((v >> 6) & 0x01), F((v >> 7) & 0x01) {}
    // Result of the last comparison was negative
    u8 N : 1 = 0;
    // Result of the last comparison was zero
    u8 Z : 1 = 0;
    // If 0, in target mode / id contains a target id
    // if 1, in register mode / id containrs a register+field ID.
    u8 TR : 1 = 0;
    // Live bit. If 0, the blaster is halted. If 1, the blaster is running.
    u8 L : 1 = 1;
    // 1 if M1/M2 are currently enabled, 0 otherwise. When setting M1 or M2 to 0, the associated register should be
    // cleared to. Access to a disabled register must not fault.
    u8 M1 : 1 = 0;
    u8 M2 : 1 = 0;
    // If 1, clear MOD1/MOD2 at the start of the next instruction.
    u8 CLRMOD : 1 = 0;
    // Set to 1 if the last memory access failed. The blaster will not halt, but someone should check this flag!
    u8 F : 1 = 0;
    u8 as_u8() const {
      return (N << 0) | (Z << 1) | (TR << 2) | (L << 3) | (M1 << 4) | (M2 << 5) | (CLRMOD << 6) | (F << 7);
    }
  };

  struct State {
    // Contain the 2-byte aligned instruction pointer. hi contains buffer ptr, lo contains offset.
    tvm::SegmentPair IP{};
    // Some instructions  process data. hi contains that data's buffer ptr, and lo is an offset into that buffer.
    tvm::SegmentPair DP{};
    // Length of data at DP in bytes
    u16 DS = 0;
    // Contains the access kind for next memory access
    u16 ACCESS = 0;
    // If in target mode, lo contains the 16-bit ID of a target device
    // If in register mode, hi is the 16-bit register ID and lo is 16-bit field ID.
    tvm::SegmentPair ID = {};
    // If in target mode, hi contains the 16-bit offset into the target's address space, and lo contains the 16-bit
    // offset. If in register mode, unused.
    tvm::SegmentPair OFF = {};
    // Modifiers register, whose meaning depends on the instruction being executed
    tvm::SegmentPair MOD1 = {}, MOD2 = {};
    /*
     * None of the following registers are accessible vis load-masked-register
     */
    // When the machine is stopped, indicated the reason why.
    // If None, then the machine is either running or stopped normally and can be resumed easily.
    // If set to a cause, you need to address the underlying reason before resuming.
    tvm::StopCause STOP_CAUSE = tvm::StopCause::None;
    // Stack pointer, used to make call/ret work.
    u16 SP = 0;
    // 16-bit integer which contains a decoded instruction.
    tvm::OpWord IS{};
  };

  // Yes, this pays an indirect call with some trampoline magic, but it allows the register blaster and trace buffer to
  // become the same class. That execution speed penalty is more than worth it to me to consolidate the two types.
  using CMPCallback = std::function<void(tvm::Interpreter &, bool)>;
  // Non-owning pointer to system.
  Interpreter(std::shared_ptr<pepp::bts::BufferManager> mgr, System *system = nullptr);
  // Disable copy/move since this class is EXPENSIVE
  Interpreter(const Interpreter &) = delete;
  Interpreter(Interpreter &&) = delete;
  Interpreter &operator=(const tvm::Interpreter &) = delete;
  Interpreter &operator=(tvm::Interpreter &&) = delete;

  void update_ip(pepp::bts::Buffer::Location loc);
  void update_ip(pepp::bts::Buffer::ID, u16 offset = 0);
  // Assuming some code is already under IP, try to run it!
  void step();
  // Update IP to point to loc, then call step() in a loop while L==1.
  // Each program executed this way must terminate with a HALT.
  // At the end of a call to run_direct, L is always 0.
  void run_direct(pepp::bts::Buffer::Location loc);
  // For each buffer location set L=1 and call run_direct.
  // Only stops when reaching the end of this buffer, or on "hard stop", where L==0 && F==1.
  void run_indirect(std::span<pepp::bts::Buffer::Location> locs);
  // Iterator-pair variant: works with TraceBuffer::Iterator, reverse iterators, etc.
  // Declared as a template to avoid include'ing TraceBuffer in this header
  template <typename It>
  void run_indirect(It begin, It end) {
    for (auto it = begin; it != end; ++it) {
      run_direct(*it);
      if (_csrs.F == 1) break;
    }
  }
  u16 register_cmp_callback(CMPCallback cb) {
    u16 id = _cmp_callbacks.size();
    _cmp_callbacks.push_back(cb);
    return id;
  }
  auto &csrs() { return _csrs; }
  const auto &csrs() const { return _csrs; }
  auto &regs() { return _regs; }
  const auto &regs() const { return _regs; }
  pepp::bts::BufferManager &mgr() { return *_mgr; }
  const pepp::bts::BufferManager &mgr() const { return *_mgr; }
  void set_trace_buffer(tvm::TraceBuffer *tb) { _tb = tb; }
  tvm::TraceBuffer *trace_buffer() const { return _tb; }
  bool stopped() const { return _csrs.L == 0; }
  // Why the machine stopped. Distinguish hard/soft stop with F bit. A normal exit uses StopCause::None && F==0.
  tvm::StopCause stop_cause() const { return _regs.STOP_CAUSE; }
  // The most recently decoded instruction, with all of its operands already resolved.
  const tvm::DecodedOp::OpChoice &decoded() const { return _decoded; }

protected:
  void soft_stop(tvm::StopCause cause = tvm::StopCause::None);
  void hard_stop(tvm::StopCause cause = tvm::StopCause::None);
  // Takes in a decoded opcode and dispatches to the appropriate execute* function.
  void execute();
  void execute_halt(tvm::DecodedOp::Halt op);
  void execute_ret(tvm::DecodedOp::Ret op);
  void execute_call(tvm::DecodedOp::Call op);
  void execute_invcall(tvm::DecodedOp::InvCall op);
  void execute_asyn(tvm::DecodedOp::ASyn op);
  void execute_isyn(tvm::DecodedOp::ISyn op);
  void execute_lmr(tvm::DecodedOp::LMR op);
  void execute_br(tvm::DecodedOp::BR op);
  void execute_setmem(tvm::DecodedOp::SetMem op);
  void execute_cmpmem(tvm::DecodedOp::CmpMem op);
  void execute_clrmem(tvm::DecodedOp::ClrMem op);
  void execute_setreg(tvm::DecodedOp::SetReg op);
  void execute_cmpreg(tvm::DecodedOp::CmpReg op);
  void execute_clrreg(tvm::DecodedOp::ClrReg op);
  void execute_traddr(tvm::DecodedOp::TRADDR op);
  void execute_ldp(tvm::DecodedOp::LDP op);
  void execute_dpincr(tvm::DecodedOp::DPIncr op);

private:
  // Fetch the word under IP, increment the IP, and set the registers & flags according to the decoded opcode.
  // If the operation is not a simple register write, then store all of the register info needed to execute it in one of
  // the DecodedOp variants.
  void decode();

  tvm::DecodedOp::Halt decode_halt(pepp::bts::Buffer::ID ibp, u16 iop);
  // Register write is result of stackop, which is not allowed in decode stage
  tvm::DecodedOp::Ret decode_ret(pepp::bts::Buffer::ID ibp, u16 iop);
  // Register write depends on a preceding stack op, which is not allowed in decode stage.
  tvm::DecodedOp::Call decode_call(pepp::bts::Buffer::ID ibp, u16 iop);
  tvm::DecodedOp::InvCall decode_invcall(pepp::bts::Buffer::ID ibp, u16 iop);
  tvm::DecodedOp::ASyn decode_asyn(pepp::bts::Buffer::ID ibp, u16 iop);
  tvm::DecodedOp::ISyn decode_isyn(pepp::bts::Buffer::ID ibp, u16 iop);
  // Shared operand decoding for ASYN/ISYN. Programs the MOD registers for the immediate form, then reads the
  // little-endian timestamp bytes. `width` receives the number of bytes actually consumed so that the caller can
  // sign-extend a delta; the returned value itself is only zero-extended.
  u64 decode_syn_data(pepp::bts::Buffer::ID ibp, u16 iop, u8 &width);
  // Unlike other decode functions, this one does not update registers!
  // This is because the shift/extract logic is somewhat complex -- and really belongs in the execute stage.
  tvm::DecodedOp::LMR decode_lmr(pepp::bts::Buffer::ID ibp, u16 iop);
  tvm::DecodedOp::BR decode_br(pepp::bts::Buffer::ID ibp, u16 iop);
  tvm::DecodedOp::SetMem decode_setmem(pepp::bts::Buffer::ID ibp, u16 iop);
  tvm::DecodedOp::CmpMem decode_cmpmem(pepp::bts::Buffer::ID ibp, u16 iop);
  tvm::DecodedOp::ClrMem decode_clrmem(pepp::bts::Buffer::ID ibp, u16 iop);
  tvm::DecodedOp::SetReg decode_setreg(pepp::bts::Buffer::ID ibp, u16 iop);
  tvm::DecodedOp::CmpReg decode_cmpreg(pepp::bts::Buffer::ID ibp, u16 iop);
  tvm::DecodedOp::ClrReg decode_clrreg(pepp::bts::Buffer::ID ibp, u16 iop);
  tvm::DecodedOp::TRADDR decode_traddr(pepp::bts::Buffer::ID ibp, u16 iop);
  tvm::DecodedOp::LDP decode_ldp(pepp::bts::Buffer::ID ibp, u16 iop);
  tvm::DecodedOp::DPIncr decode_accdp(pepp::bts::Buffer::ID ibp, u16 iop);
  tvm::DecodedOp::DPIncr decode_incdp(pepp::bts::Buffer::ID ibp, u16 iop);

  // Perform an LE read of 2 bytes at an offset.
  u16 read16(pepp::bts::Buffer::ID, u16 offset);
  // SP -= 4 and return the 4 bytes. IF SP would underflow stack, set L=0 and set cause in MOD1.lo
  tvm::SegmentPair pop();
  // SP +=4 and write the 4 bytes. If SP would overflow stack, set L = 0 and set cause in MOD1.lo
  void push(tvm::SegmentPair v);
  std::shared_ptr<pepp::bts::BufferManager> _mgr;
  System *_system = nullptr;
  tvm::TraceBuffer *_tb = nullptr;
  RegisterScan *_scan;
  std::array<u8, 256> _stack;
  // Should really be a map so you can delete callbacks, but I can't be bothered to add the ID variable right now.
  std::vector<CMPCallback> _cmp_callbacks;
  Flags _csrs{};
  State _regs{};
  std::vector<u8> _tmp;
  tvm::DecodedOp::OpChoice _decoded{};
};
} // namespace tvm