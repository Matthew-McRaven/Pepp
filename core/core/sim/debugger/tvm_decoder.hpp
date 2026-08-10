#pragma once
#include <memory>
#include "core/ds/alloc/pagechain.hpp"
#include "core/integers.h"
#include "core/sim/debugger/tvm_encoding.hpp"
#include "core/sim/debugger/tvm_machine.hpp"

namespace tvm {

// Turns the instruction packet under IP into a fully-resolved DecodedOp, and advances IP past it.
//
// Decoding is deliberately NOT a pure function of the packet. The ISA retains register state across instructions  so
// cracking a packet involves combining current register state with the incoming packet's state.
// All users of this ISA need identical decoding, which is why it is factored out into a shared class.
//
// Because decoding writes registers, a Decoder holds a mutable reference to the same MachineState its backend uses.
// It can also fail: an unreadable instruction buffer, an out-of-range DP for a sync op, or an unknown opcode all
// hard-stop the machine, so the caller must re-check L before executing what was decoded.
class Decoder {
public:
  Decoder(std::shared_ptr<pepp::bts::BufferManager> mgr, MachineState &state);

  // Fetch the word under IP, advance IP past the packet, program the registers this packet supplied, and resolve the
  // operands into decoded(). Check state.csrs.L afterwards: a decode failure leaves decoded() holding the *previous*
  // instruction's alternative.
  void decode();

  // The most recently decoded instruction, with all of its operands already resolved. Callers that want to observe a
  // program without running it (timestamps, touched locations) can read this between decode and execute.
  const tvm::DecodedOp::OpChoice &decoded() const { return _decoded; }

private:
  tvm::DecodedOp::Halt decode_halt(pepp::bts::Buffer::ID ibp, u16 iop);
  // Register write is result of stackop, which is not allowed in decode stage
  tvm::DecodedOp::Ret decode_ret(pepp::bts::Buffer::ID ibp, u16 iop);
  // Register write depends on a preceding stack op, which is not allowed in decode stage.
  tvm::DecodedOp::Call decode_call(pepp::bts::Buffer::ID ibp, u16 iop);
  tvm::DecodedOp::InvCall decode_invcall(pepp::bts::Buffer::ID ibp, u16 iop);
  // Operand-free, like RET. Kept separate so dispatch routes it to on_invret rather than on_ret.
  tvm::DecodedOp::InvRet decode_invret(pepp::bts::Buffer::ID ibp, u16 iop);
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
  // Resolves to the same DecodedOp::SetMem as decode_setmem -- the only difference is where the offset came from, and
  // by the time a backend sees it that distinction has already been resolved away. That is why there is no matching
  // on_setmemdx handler and no new variant alternative.
  tvm::DecodedOp::SetMem decode_setmemdx(pepp::bts::Buffer::ID ibp, u16 iop);
  tvm::DecodedOp::CmpMem decode_cmpmem(pepp::bts::Buffer::ID ibp, u16 iop);
  tvm::DecodedOp::ClrMem decode_clrmem(pepp::bts::Buffer::ID ibp, u16 iop);
  tvm::DecodedOp::SetReg decode_setreg(pepp::bts::Buffer::ID ibp, u16 iop);
  tvm::DecodedOp::CmpReg decode_cmpreg(pepp::bts::Buffer::ID ibp, u16 iop);
  tvm::DecodedOp::ClrReg decode_clrreg(pepp::bts::Buffer::ID ibp, u16 iop);
  tvm::DecodedOp::TRADDR decode_traddr(pepp::bts::Buffer::ID ibp, u16 iop);
  tvm::DecodedOp::LDP decode_ldp(pepp::bts::Buffer::ID ibp, u16 iop);
  tvm::DecodedOp::DPIncr decode_accdp(pepp::bts::Buffer::ID ibp, u16 iop);
  tvm::DecodedOp::DPIncr decode_incdp(pepp::bts::Buffer::ID ibp, u16 iop);

  u16 read(pepp::bts::Buffer::ID id, u16 offset) { return tvm::read16(*_mgr, _state, id, offset); }

  std::shared_ptr<pepp::bts::BufferManager> _mgr;
  MachineState &_state;
  tvm::DecodedOp::OpChoice _decoded{};
};

} // namespace tvm
