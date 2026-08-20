#include "pep_isa_instructions.hpp"
#include "core/sim/cores/cpu/pep_isa.hpp"
#include "core/sim/memory/ram/dense.hpp"

u8 pack_csr(bool n, bool z, bool v, bool c) {
  u8 nzvc = 0;
  if (n) nzvc |= 1 << 3;
  if (z) nzvc |= 1 << 2;
  if (v) nzvc |= 1 << 1;
  if (c) nzvc |= 1 << 0;
  return nzvc;
}

std::tuple<bool, bool, bool, bool> unpack_csrs(u8 nzvc) {
  bool n = nzvc & (1 << 3);
  bool z = nzvc & (1 << 2);
  bool v = nzvc & (1 << 1);
  bool c = nzvc & (1 << 0);
  return {n, z, v, c};
}

u16 decode_op_addr(PepISA3CPU *self, isa::SharedAddrMode addr) {
  // Fetch current PC
  u16 pc = self->read_pc();
  // Increment PC by 2 to point to next instruction.
  self->write_pc(pc + 2);
  auto target = self->target();
  // Read value at mem[PC] into OS register.
  u16 opr = target->read<u16, bits::host_is_le>(pc, self->op_data()).second;
  self->write_register(isa::Pep10::Register::OS, opr);

  switch (addr) {
  case isa::SharedAddrMode::I: return pc;
  case isa::SharedAddrMode::N: opr = target->read<u16, bits::host_is_le>(opr, self->op_data()).second; [[fallthrough]];
  case isa::SharedAddrMode::D: return opr;

  case isa::SharedAddrMode::SF:
    opr = self->read_register(isa::Pep10::Register::SP) + opr;
    return self->target()->read<u16, bits::host_is_le>(opr, self->op_data()).second;

  case isa::SharedAddrMode::S: return self->read_register(isa::Pep10::Register::SP) + opr;
  case isa::SharedAddrMode::X: return self->read_register(isa::Pep10::Register::X) + opr;
  case isa::SharedAddrMode::SX:
    return self->read_register(isa::Pep10::Register::X) + self->read_register(isa::Pep10::Register::SP) + opr;
  case isa::SharedAddrMode::SFX:
    opr = self->read_register(isa::Pep10::Register::SP) + opr;
    return self->read_register(isa::Pep10::Register::X) + self->target()->read<u16, bits::host_is_le>(opr, self->op_data()).second;
  }
  throw std::logic_error("Invalid addressing mode for decode_op_addr");
}

void unimpl_handler(PepISA3CPU *) { throw std::logic_error("Unimplemented instruction encountered"); }

void handle_ret(PepISA3CPU *self) {
  self->decrement_call_depth();
  u16 sp = self->read_register(isa::Pep10::Register::SP);
  auto addr = self->target()->read<u16, bits::host_is_le>(sp, self->op_data()).second;
  self->write_pc(addr);
  self->write_register(isa::Pep10::Register::SP, sp + 2);
  // TODO: notify debugger of ret @ PC
}

void handle_sret(PepISA3CPU *self) {
  // Long enough to either hold all regs or one ctx switch block.
  static constexpr u8 registersBytes = 2 * ::isa::Pep10::RegisterCount;
  u8 ctx[std::max<std::size_t>(registersBytes, 12)];
  auto ctxSpan = bits::span<u8>{ctx, sizeof(ctx)};

  auto memory = self->target();
  // Fill ctx with all register's current values.
  // Then we can do a single write back to _regs and only generate 1 trace
  // packet.
  auto regs = self->registers();
  u16 sp = self->read_register(isa::Pep10::Register::SP);
  u16 tmp = size_inclusive(regs->span());
  regs->read(0, {ctx, tmp}, self->op_data());

  // Reload NZVC
  auto csrs = memory->read<u8>(sp, self->op_data()).second;
  self->write_packed_csr(csrs);

  // Load A into ctx. No need for byteswap, _memory is little endian as are
  // regs.
  memory->read(sp + 1, {ctx + 2 * static_cast<u8>(isa::Pep10::Register::A), 2}, self->op_data());

  // Load X into ctx
  memory->read(sp + 3, {ctx + 2 * static_cast<u8>(isa::Pep10::Register::X), 2}, self->op_data());

  // Load PC into ctx
  memory->read(sp + 5, {ctx + 2 * static_cast<u8>(isa::Pep10::Register::PC), 2}, self->op_data());

  // Load SP into ctx
  memory->read(sp + 7, {ctx + 2 * static_cast<u8>(isa::Pep10::Register::SP), 2}, self->op_data());

  // Bulk write-back regs, saving a number of bits on trace metadata.
  regs->write(0, {ctx, registersBytes}, self->op_data());
  // That write covered PC, restoring it from the stack. Hand it to the working copy, or clock_tick's single store
  // would put the pre-instruction value straight back over it. Read it back through the bank rather than picking it
  // out of ctx so this stays independent of the context block's layout and byte order.
  self->write_pc(self->read_register(isa::Pep10::Register::PC));

  tmp = sp + 12;
  // Using "host"'s variables, so byte swap if necessary.
  if (bits::host_is_le) tmp = bits::byteswap(tmp);
  memory->write(static_cast<u16>(::isa::Pep10::MemoryVectors::SystemStackPtr), {reinterpret_cast<u8 *>(&tmp), 2}, self->op_data());

  self->decrement_call_depth();
  if (false) {
    //_dbg->bps->notifyPCChanged(readReg(Register::PC));
    //_dbg->notifyTrapRet(pc - 1, readReg(Register::SP));
  }
  // Skip "normal" return path, since we've already written to PC.
}

void handle_movflga(PepISA3CPU *self) {
  auto nzvc = self->read_packed_csr();
  self->write_register(isa::Pep10::Register::A, nzvc);
}

void handle_movaflg(PepISA3CPU *self) {
  auto nzvc = self->read_register(isa::Pep10::Register::A);
  self->write_packed_csr(nzvc);
}

void handle_movspa(PepISA3CPU *self) {
  auto sp = self->read_register(isa::Pep10::Register::SP);
  self->write_register(isa::Pep10::Register::A, sp);
}

void handle_movasp(PepISA3CPU *self) {
  auto a = self->read_register(isa::Pep10::Register::A);
  self->write_register(isa::Pep10::Register::SP, a);
}

void handle_nop(PepISA3CPU *) {}

void handle_negr(PepISA3CPU *self, isa::Pep10::Register reg) {
  u16 src = self->read_register(reg);
  u16 tmp = ~src + 1;
  bool n = tmp & 0x8000;
  bool z = tmp == 0x0000;
  bool v = tmp == 0x8000;
  bool c = src == 0x0000;
  self->write_register(reg, tmp);
  self->write_packed_csr(pack_csr(n, z, v, c));
}

void handle_aslr(PepISA3CPU *self, isa::Pep10::Register reg) {
  auto src = self->read_register(reg);
  // Store in temp, because we need acc for status bit computation.
  u16 tmp = src << 1;
  // Is negative if high order bit is 1.
  bool n = tmp & 0x8000;
  // Is zero if all bits are 0's.
  bool z = tmp == 0x0000;
  // Signed overflow occurs when the starting & ending values of the high
  // order bit differ (a xor temp == 1). Then shift the result over by 15
  // places to only keep high order bit (which is the sign).
  bool v = (src ^ tmp) >> 15;
  // Carry out if register starts with high order 1.
  bool c = src & 0x8000;
  self->write_register(reg, tmp);
  self->write_packed_csr(pack_csr(n, z, v, c));
}

void handle_asrr(PepISA3CPU *self, isa::Pep10::Register reg) {
  auto src = self->read_register(reg);
  // Shift all bits to the right by 1 position. Since using unsigned shift,
  // must explicitly perform sign extension by hand.
  u16 tmp = static_cast<u16>(src >> 1 | ((src & 0x8000) ? 1 << 15 : 0));
  // Is negative if high order bit is 1.
  bool n = tmp & 0x8000;
  // Is zero if all bits are 0's.
  bool z = tmp == 0x000;
  // Carry out if register starts with low order 1.
  bool c = src & 0x1;
  bool v = 0;
  self->write_register(reg, tmp);
  self->write_packed_csr(pack_csr(n, z, v, c));
}

void handle_notr(PepISA3CPU *self, isa::Pep10::Register reg) {
  auto src = self->read_register(reg);
  auto [n, z, v, c] = unpack_csrs(self->read_packed_csr());
  u16 tmp = ~src;
  n = tmp & 0x8000;
  z = tmp == 0x0000;
  self->write_register(reg, tmp);
  self->write_packed_csr(pack_csr(n, z, v, c));
}

void handle_rolr(PepISA3CPU *self, isa::Pep10::Register reg) {
  auto src = self->read_register(reg);
  auto [n, z, v, c] = unpack_csrs(self->read_packed_csr());
  // Shift the carry in to low order bit.
  u16 tmp = static_cast<u16>(src << 1 | (c ? 1 : 0));
  n = tmp & 0x8000;
  z = tmp == 0x0000;
  // Carry out if register starts with high order 1.
  c = src & 0x8000;
  self->write_register(reg, tmp);
  self->write_packed_csr(pack_csr(n, z, v, c));
}

void handle_rorr(PepISA3CPU *self, isa::Pep10::Register reg) {
  auto src = self->read_register(reg);
  auto [n, z, v, c] = unpack_csrs(self->read_packed_csr());
  // Shift the carry in to high order bit.
  u16 tmp = src >> 1 | (c ? 1 << 15 : 0);
  n = tmp & 0x8000;
  z = tmp == 0x0000;
  // Carry out if register starts with low order 1.
  c = src & 0x1;
  self->write_register(reg, tmp);
  self->write_packed_csr(pack_csr(n, z, v, c));
}

void handle_branch(PepISA3CPU *self, Op op, BranchCondition cond, u16 op_addr) {
  const auto [n, z, v, c] = unpack_csrs(self->read_packed_csr());
  const u16 op_spec = self->target()->read<u16, bits::host_is_le>(op_addr, self->op_data()).second;
  bool taken;
  switch (cond) {
  case BranchCondition::UNCONDITIONAL: taken = true; break;
  case BranchCondition::LE: taken = n || z; break;
  case BranchCondition::LT: taken = n; break;
  case BranchCondition::EQ: taken = z; break;
  case BranchCondition::NE: taken = !z; break;
  case BranchCondition::GE: taken = !n; break;
  case BranchCondition::GT: taken = !n && !z; break;
  case BranchCondition::V: taken = v; break;
  case BranchCondition::C: taken = c; break;
  }
  if (taken) self->write_pc(op_spec);
}

void handle_unconditional_branch(PepISA3CPU *self, Op op, u16 op_addr) {
  const u16 op_spec = self->target()->read<u16, bits::host_is_le>(op_addr, self->op_data()).second;
  self->write_pc(op_spec);
}

void handle_call(PepISA3CPU *self, Op op, u16 op_addr) {
  const u16 op_spec = self->target()->read<u16, bits::host_is_le>(op_addr, self->op_data()).second;
  const u16 pc = self->read_pc();
  u16 sp = self->read_register(isa::Pep10::Register::SP);
  self->target()->write<u16, bits::host_is_le>(sp -= 2, pc, self->op_data());
  self->write_register(isa::Pep10::Register::SP, sp);
  self->write_pc(op_spec);
  self->increment_call_depth();
  // TODO: if (_dbg) _dbg->notifyCall(pc - 3, sp);
}

void handle_addsp(PepISA3CPU *self, Op op, u16 op_addr) {
  const u16 op_spec = self->target()->read<u16, bits::host_is_le>(op_addr, self->op_data()).second;
  const auto sp = self->read_register(isa::Pep10::Register::SP) + op_spec;
  self->write_register(isa::Pep10::Register::SP, sp);
  // TODO: if (_dbg) _dbg->notifyAddSP(pc - 3, sp);
}

void handle_subsp(PepISA3CPU *self, Op op, u16 op_addr) {
  const u16 op_spec = self->target()->read<u16, bits::host_is_le>(op_addr, self->op_data()).second;
  const auto sp = self->read_register(isa::Pep10::Register::SP) - op_spec;
  self->write_register(isa::Pep10::Register::SP, sp);
  // TODO: if (_dbg) _dbg->notifySubSP(pc - 3, sp);
}

void handle_addr(PepISA3CPU *self, Op op, u16 op_addr) {
  const isa::Pep10::Register reg = static_cast<isa::Pep10::Register>(op.target);
  const u16 op_spec = self->target()->read<u16, bits::host_is_le>(op_addr, self->op_data()).second;
  const u16 src = self->read_register(reg);
  const u16 tmp = src + op_spec;
  // Is negative if high order bit is 1.
  bool n = tmp & 0x8000;
  // Is zero if all bits are 0's.
  bool z = tmp == 0x0000;
  // There is a signed overflow iff the high order bits of the register
  // and operand are the same, and one input & the output differ in sign.
  // >> Shifts in 0's (unsigned shorts), so after shift, only high order
  // bit remain.
  bool v = (~(src ^ op_spec) & (src ^ tmp)) >> 15;
  // Carry out iff result is unsigned less than register or operand.
  bool c = tmp < src || tmp < op_spec;
  self->write_register(reg, tmp);
  self->write_packed_csr(pack_csr(n, z, v, c));
}

void handle_subr(PepISA3CPU *self, Op op, u16 op_addr) {
  const isa::Pep10::Register reg = static_cast<isa::Pep10::Register>(op.target);
  const u16 operand = self->target()->read<u16, bits::host_is_le>(op_addr, self->op_data()).second;
  const u16 src = self->read_register(reg);
  const u16 tmp = src + ~operand + 1;
  // Is negative if high order bit is 1.
  bool n = tmp & 0x8000;
  // Is zero if all bits are 0's.
  bool z = tmp == 0x0000;
  // There is a signed overflow iff the high order bits of the register
  // and operand are the same, and one input & the output differ in sign.
  // >> Shifts in 0's (unsigned shorts), so after shift, only high order
  // bit remain.
  bool v = (~(src ^ (~operand + 1)) & (src ^ tmp)) >> 15;
  // Carry out iff result is unsigned less than register or operand.
  bool c = tmp < src || tmp < static_cast<u16>(1 + ~operand);
  self->write_register(reg, tmp);
  self->write_packed_csr(pack_csr(n, z, v, c));
}

void handle_bitopr(PepISA3CPU *self, Op op, Bitop bitop, u16 op_addr) {
  const isa::Pep10::Register reg = static_cast<isa::Pep10::Register>(op.target);
  const u16 op_spec = self->target()->read<u16, bits::host_is_le>(op_addr, self->op_data()).second;
  const u16 src = self->read_register(reg);
  auto [n, z, v, c] = unpack_csrs(self->read_packed_csr());
  u16 tmp;
  switch (bitop) {
  case Bitop::AND: tmp = src & op_spec; break;
  case Bitop::OR: tmp = src | op_spec; break;
  case Bitop::XOR: tmp = src ^ op_spec; break;
  }
  // Is negative if high order bit is 1.
  n = tmp & 0x8000;
  // Is zero if all bits are 0's.
  z = tmp == 0x0000;
  self->write_register(reg, tmp);
  self->write_packed_csr(pack_csr(n, z, v, c));
}

void handle_cpwr(PepISA3CPU *self, Op op, u16 op_addr) {
  const isa::Pep10::Register reg = static_cast<isa::Pep10::Register>(op.target);
  const u16 operand = self->target()->read<u16, bits::host_is_le>(op_addr, self->op_data()).second;
  const u16 src = self->read_register(reg);
  const u16 neg = ~operand + 1;
  const u16 tmp = src + neg;
  // Is negative if high order bit is 1.
  bool n = tmp & 0x8000;
  // Is zero if all bits are 0's.
  bool z = tmp == 0x0000;
  // There is a signed overflow iff the high order bits of the register
  // and operand are the same, and one input & the output differ in sign.
  // >> Shifts in 0's (unsigned shorts), so after shift, only high order
  // bit remain.
  bool v = (~(src ^ neg) & (src ^ tmp)) >> 15;
  // Carry out iff result is unsigned less than register or operand.
  bool c = tmp < src || tmp < neg;
  // Invert N bit if there was signed overflow.
  n ^= v;
  self->write_packed_csr(pack_csr(n, z, v, c));
}

void handle_cpbr(PepISA3CPU *self, Op op, u16 op_addr) {
  const isa::Pep10::Register reg = static_cast<isa::Pep10::Register>(op.target);
  // op_addr is address for 2-byte operands, so we need an offset of 1.
  const u8 op_spec = self->target()->read<u8>(op_addr + 1, self->op_data()).second;
  const auto src = self->read_register(reg);
  // The result is the decoded operand specifier plus A/X. mask down to a byte.
  u16 tmp = (src + ~op_spec + 1) & 0xff;
  // Is negative if high order bit is 1.
  bool n = tmp & 0x80;
  // Is zero if all bits are 0's.
  bool z = tmp == 0x00;
  // RTL specifies that VC get 0.
  self->write_packed_csr(pack_csr(n, z, 0, 0));
}

void handle_ldwr(PepISA3CPU *self, Op op, u16 op_addr) {
  const isa::Pep10::Register reg = static_cast<isa::Pep10::Register>(op.target);
  const u16 op_spec = self->target()->read<u16, bits::host_is_le>(op_addr, self->op_data()).second;
  auto [n, z, v, c] = unpack_csrs(self->read_packed_csr());
  // Is negative if high order bit is 1.
  n = op_spec & 0x8000;
  z = op_spec == 0x0000;

  self->write_register(reg, op_spec);
  self->write_packed_csr(pack_csr(n, z, v, c));
}

void handle_ldbr(PepISA3CPU *self, Op op, u16 op_addr) {
  const isa::Pep10::Register reg = static_cast<isa::Pep10::Register>(op.target);
  // op_addr is address for 2-byte operands, so we need an offset of 1.
  const u8 op_spec = self->target()->read<u8>(op_addr + 1, self->op_data()).second;
  auto [n, z, v, c] = unpack_csrs(self->read_packed_csr());
  // LDBr always clears n.
  n = 0;
  z = op_spec == 0x0000;

  self->write_register(reg, op_spec);
  self->write_packed_csr(pack_csr(n, z, v, c));
}

void handle_stwr(PepISA3CPU *self, Op op, u16 op_addr) {
  const isa::Pep10::Register reg = static_cast<isa::Pep10::Register>(op.target);
  u16 src = self->read_register(reg);
  self->target()->write<u16, bits::host_is_le>(op_addr, src, self->op_data());
}

void handle_stbr(PepISA3CPU *self, Op op, u16 op_addr) {
  const isa::Pep10::Register reg = static_cast<isa::Pep10::Register>(op.target);
  const u8 src = self->read_register(reg);
  self->target()->write<u8>(op_addr, src, self->op_data());
}
