#include "pep_isa_instructions.hpp"
#include "core/sim/cores/cpu/pep/pep_csrbank.hpp"
#include "core/sim/cores/cpu/pep/pep_isa.hpp"

// The two ISAs declare identical Register enums, so one alias serves both.
using R = isa::Pep10::Register;

u16 decode_op_addr(PepISA3CPU *self, isa::SharedAddrMode addr) {
  auto target = self->target();
  auto opdata = self->op_data();
  // Fetch current PC
  u16 pc = self->read_pc();
  // Increment PC by 2 to point to next instruction.
  self->write_pc(pc + 2);
  // Read value at mem[PC] into OS register.
  u16 opr = target->read<u16, bits::host_is_le>(pc, opdata).second;
  self->write_register<R::OS>(opr);
  switch (addr) {
  case isa::SharedAddrMode::I: return pc;
  case isa::SharedAddrMode::N: opr = target->read<u16, bits::host_is_le>(opr, opdata).second; [[fallthrough]];
  case isa::SharedAddrMode::D: break;
  case isa::SharedAddrMode::SF:
    opr = self->read_register<R::SP>() + opr;
    opr = target->read<u16, bits::host_is_le>(opr, opdata).second;
    break;
  case isa::SharedAddrMode::S: opr = self->read_register<R::SP>() + opr; break;
  case isa::SharedAddrMode::X: opr = self->read_register<R::X>() + opr; break;
  case isa::SharedAddrMode::SX: opr = self->read_register<R::X>() + self->read_register<R::SP>() + opr; break;
  case isa::SharedAddrMode::SFX:
    opr = self->read_register<R::SP>() + opr;
    opr = self->read_register<R::X>() + target->read<u16, bits::host_is_le>(opr, opdata).second;
    break;
  default: throw std::logic_error("Invalid addressing mode for decode_op_addr");
  }
  return opr;
}

// Nearly identical to decode_op_addr, except we perform an extra level memory load before returning.
template <std::unsigned_integral T> T decode_op_value(PepISA3CPU *self, isa::SharedAddrMode addr) {
  auto target = self->target();
  auto opdata = self->op_data();
  // Fetch current PC
  u16 pc = self->read_pc();
  // Increment PC by 2 to point to next instruction.
  self->write_pc(pc + 2);
  // Read value at mem[PC] into OS register.
  u16 opr = target->read<u16, bits::host_is_le>(pc, opdata).second;
  self->write_register<R::OS>(opr);
  switch (addr) {
  case isa::SharedAddrMode::I: return opr;
  case isa::SharedAddrMode::N: opr = target->read<u16, bits::host_is_le>(opr, opdata).second; [[fallthrough]];
  case isa::SharedAddrMode::D: break;
  case isa::SharedAddrMode::SF:
    opr = self->read_register<R::SP>() + opr;
    opr = target->read<u16, bits::host_is_le>(opr, opdata).second;
    break;
  case isa::SharedAddrMode::S: opr = self->read_register<R::SP>() + opr; break;
  case isa::SharedAddrMode::X: opr = self->read_register<R::X>() + opr; break;
  case isa::SharedAddrMode::SX: opr = self->read_register<R::X>() + self->read_register<R::SP>() + opr; break;
  case isa::SharedAddrMode::SFX:
    opr = self->read_register<R::SP>() + opr;
    opr = self->read_register<R::X>() + target->read<u16, bits::host_is_le>(opr, opdata).second;
    break;
  default: throw std::logic_error("Invalid addressing mode for decode_op_addr");
  }
  return target->read<T, bits::host_is_le>(opr, opdata).second;
}

u16 decode_op_value_word(PepISA3CPU *self, isa::SharedAddrMode addr) { return decode_op_value<u16>(self, addr); }
u8 decode_op_value_byte(PepISA3CPU *self, isa::SharedAddrMode addr) { return decode_op_value<u8>(self, addr); }

void unimpl_handler(PepISA3CPU *) { throw std::logic_error("Unimplemented instruction encountered"); }

void handle_ret(PepISA3CPU *self) {
  self->decrement_call_depth();
  u16 sp = self->read_register<R::SP>();
  auto addr = self->target()->read<u16, bits::host_is_le>(sp, self->op_data()).second;
  self->write_pc(addr);
  self->write_register<R::SP>(sp + 2);
  // TODO: notify debugger of ret @ PC
}

void handle_sret(PepISA3CPU *self) {
  static constexpr u16 sys_sp_vec = static_cast<u16>(::isa::Pep10::MemoryVectors::SystemStackPtr);
  auto memory = self->target();
  auto regs = self->registers();
  u16 sp = self->read_register<R::SP>();
  u16 tmp = size_inclusive(regs->span());

  // Read all existing registers into a temporary buffer, which we will patch with the saved values of the PCB.
  static constexpr u8 registersBytes = 2 * ::isa::Pep10::RegisterCount;
  u8 ctx[std::max<std::size_t>(registersBytes, 12)];
  regs->read(0, {ctx, tmp}, self->op_data());

  // One read for the saved NZVC followed by A, X, PC, and SP.
  // write() on register bank exposes registers in BE order, so no need to byteswap.
  u8 bytes[9];
  auto pcb = bits::span<u8>{bytes, sizeof(bytes)};
  memory->read(sp, pcb, self->op_data());
  // Restore NZVC
  self->write_packed_csr(pcb[0]);
  // Copy A,X,PC,SP into the preserved registers...
  std::copy_n(bytes + 1, 4, ctx + 2 * static_cast<u8>(R::A));
  std::copy_n(bytes + 5, 2, ctx + 2 * static_cast<u8>(R::PC));
  std::copy_n(bytes + 7, 2, ctx + 2 * static_cast<u8>(R::SP));

  // ... and write back the whole register bank in bulk, saving trace metadata size
  regs->write(0, {ctx, tmp}, self->op_data());
  // Update the cached value of PC by reading it directly from the register bank
  self->write_pc(self->read_register_uncached<R::PC>());

  memory->write<u16, bits::host_is_le>(sys_sp_vec, sp + 12, self->op_data());

  self->decrement_call_depth();
  if (false) {
    //_dbg->bps->notifyPCChanged(readReg(Register::PC));
    //_dbg->notifyTrapRet(pc - 1, readReg(Register::SP));
  }
}

void handle_movflga(PepISA3CPU *self) {
  auto nzvc = self->read_packed_csr();
  self->write_register<R::A>(nzvc);
}

void handle_movaflg(PepISA3CPU *self) {
  auto nzvc = self->read_register<R::A>();
  self->write_packed_csr(nzvc);
}

void handle_movspa(PepISA3CPU *self) {
  auto sp = self->read_register<R::SP>();
  self->write_register<R::A>(sp);
}

void handle_movasp(PepISA3CPU *self) {
  auto a = self->read_register<R::A>();
  self->write_register<R::SP>(a);
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
  self->write_packed_csr(PepCSRBank::pack(n, z, v, c));
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
  self->write_packed_csr(PepCSRBank::pack(n, z, v, c));
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
  self->write_packed_csr(PepCSRBank::pack(n, z, v, c));
}

void handle_notr(PepISA3CPU *self, isa::Pep10::Register reg) {
  auto src = self->read_register(reg);
  auto [n, z, v, c] = PepCSRBank::unpack(self->read_packed_csr());
  u16 tmp = ~src;
  n = tmp & 0x8000;
  z = tmp == 0x0000;
  self->write_register(reg, tmp);
  self->write_packed_csr(PepCSRBank::pack(n, z, v, c));
}

void handle_rolr(PepISA3CPU *self, isa::Pep10::Register reg) {
  auto src = self->read_register(reg);
  auto [n, z, v, c] = PepCSRBank::unpack(self->read_packed_csr());
  // Shift the carry in to low order bit.
  u16 tmp = static_cast<u16>(src << 1 | (c ? 1 : 0));
  n = tmp & 0x8000;
  z = tmp == 0x0000;
  // Carry out if register starts with high order 1.
  c = src & 0x8000;
  self->write_register(reg, tmp);
  self->write_packed_csr(PepCSRBank::pack(n, z, v, c));
}

void handle_rorr(PepISA3CPU *self, isa::Pep10::Register reg) {
  auto src = self->read_register(reg);
  auto [n, z, v, c] = PepCSRBank::unpack(self->read_packed_csr());
  // Shift the carry in to high order bit.
  u16 tmp = src >> 1 | (c ? 1 << 15 : 0);
  n = tmp & 0x8000;
  z = tmp == 0x0000;
  // Carry out if register starts with low order 1.
  c = src & 0x1;
  self->write_register(reg, tmp);
  self->write_packed_csr(PepCSRBank::pack(n, z, v, c));
}

void handle_scall(PepISA3CPU *self) {
  static constexpr u16 sys_sp_vec = static_cast<u16>(::isa::Pep10::MemoryVectors::SystemStackPtr);
  static constexpr u16 trap_pc_vec = static_cast<u16>(::isa::Pep10::MemoryVectors::TrapHandler);
  using R = isa::Pep10::Register;
  auto target = self->target();
  u8 bytes[12];
  auto ctx = bits::span<u8>{bytes, 12};
  u16 tmp, pc = self->read_register<R::PC>();
  // Read operand specifier and increment PC. Should probably be op_instr().
  u16 os = target->read<u16, bits::host_is_le>(pc, self->op_data()).second;
  pc += 2;
  // Must byteswap because we are using "host" variables.
  ctx[0] = self->read_packed_csr();
  tmp = self->read_register<R::A>();
  ctx[1] = tmp >> 8, ctx[2] = tmp;
  tmp = self->read_register<R::X>();
  ctx[3] = tmp >> 8, ctx[4] = tmp;
  ctx[5] = pc >> 8, ctx[6] = pc;
  tmp = self->read_register<R::SP>();
  ctx[7] = tmp >> 8, ctx[8] = tmp;
  ctx[9] = self->read_register<R::IS>();
  ctx[10] = os >> 8, ctx[11] = os;

  // Read system stack address andallocate ctx frame with -=
  tmp = target->read<u16, bits::host_is_le>(sys_sp_vec, self->op_data()).second;
  target->write(tmp -= 12, ctx, self->op_data());
  self->write_register<R::SP>(tmp);

  // Read trap handler pc
  tmp = target->read<u16, bits::host_is_le>(trap_pc_vec, self->op_data()).second;
  self->write_pc(tmp);

  self->increment_call_depth();
  if (false) {
    //_dbg->bps->notifyPCChanged(readReg(Register::PC));
    //_dbg->notifyTrapCall(pc - 1, readReg(Register::SP));
  }
}
void handle_branch(PepISA3CPU *self, Op op, BranchCondition cond, u16 op_val) {
  const auto [n, z, v, c] = PepCSRBank::unpack(self->read_packed_csr());
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
  if (taken) self->write_pc(op_val);
}

void handle_unconditional_branch(PepISA3CPU *self, Op op, u16 op_val) { self->write_pc(op_val); }

void handle_call(PepISA3CPU *self, Op op, u16 op_val) {
  const u16 pc = self->read_pc();
  u16 sp = self->read_register<R::SP>();
  self->target()->write<u16, bits::host_is_le>(sp -= 2, pc, self->op_data());
  self->write_register<R::SP>(sp);
  self->write_pc(op_val);
  self->increment_call_depth();
  // TODO: if (_dbg) _dbg->notifyCall(pc - 3, sp);
}

void handle_addsp(PepISA3CPU *self, Op op, u16 op_val) {
  const auto sp = self->read_register<R::SP>() + op_val;
  self->write_register<R::SP>(sp);
  // TODO: if (_dbg) _dbg->notifyAddSP(pc - 3, sp);
}

void handle_subsp(PepISA3CPU *self, Op op, u16 op_val) {
  const auto sp = self->read_register<R::SP>() - op_val;
  self->write_register<R::SP>(sp);
  // TODO: if (_dbg) _dbg->notifySubSP(pc - 3, sp);
}

void handle_addr(PepISA3CPU *self, Op op, u16 op_val) {
  const isa::Pep10::Register reg = static_cast<isa::Pep10::Register>(op.target);
  const u16 src = self->read_register(reg);
  // Perform arithmetic at u32 so that we can test for carry out directly rather than using multiple < comparisons.
  const u32 tmp_ext = u32(src) + u32(op_val);
  const u16 tmp = tmp_ext;
  // Is negative if high order bit is 1.
  bool n = tmp & 0x8000;
  // Is zero if all bits are 0's.
  bool z = tmp == 0x0000;
  // There is a signed overflow iff the high order bits of the register
  // and operand are the same, and one input & the output differ in sign.
  // >> Shifts in 0's (unsigned shorts), so after shift, only high order
  // bit remain.
  bool v = (~(src ^ op_val) & (src ^ tmp)) >> 15;
  // Carry out iff result is unsigned less than register or operand.
  bool c = tmp_ext & 0x1'0000;
  self->write_register(reg, tmp);
  self->write_packed_csr(PepCSRBank::pack(n, z, v, c));
}

void handle_subr(PepISA3CPU *self, Op op, u16 op_val) {
  const isa::Pep10::Register reg = static_cast<isa::Pep10::Register>(op.target);
  const u16 src = self->read_register(reg);
  // Perform bitwise negation at u16 to match how the underlying HW adder works.
  // Perform arithmetic at u32 so that we can test for carry out directly rather than using multiple < comparisons.
  const u32 tmp_ext = 1_u32 + u32(src) + u16(~op_val);
  const u16 tmp = tmp_ext;
  // Is negative if high order bit is 1.
  bool n = tmp & 0x8000;
  // Is zero if all bits are 0's.
  bool z = tmp == 0x0000;
  // There is a signed overflow iff the high order bits of the register
  // and operand are the same, and one input & the output differ in sign.
  // >> Shifts in 0's (unsigned shorts), so after shift, only high order
  // bit remain.
  bool v = (~(src ^ (~op_val + 1)) & (src ^ tmp)) >> 15;
  // Carry out iff tmp[16] == 1
  bool c = tmp_ext & 0x1'0000;
  self->write_register(reg, tmp);
  self->write_packed_csr(PepCSRBank::pack(n, z, v, c));
}

void handle_bitopr(PepISA3CPU *self, Op op, Bitop bitop, u16 op_val) {
  const isa::Pep10::Register reg = static_cast<isa::Pep10::Register>(op.target);
  const u16 src = self->read_register(reg);
  auto [n, z, v, c] = PepCSRBank::unpack(self->read_packed_csr());
  u16 tmp;
  switch (bitop) {
  case Bitop::AND: tmp = src & op_val; break;
  case Bitop::OR: tmp = src | op_val; break;
  case Bitop::XOR: tmp = src ^ op_val; break;
  }
  // Is negative if high order bit is 1.
  n = tmp & 0x8000;
  // Is zero if all bits are 0's.
  z = tmp == 0x0000;
  self->write_register(reg, tmp);
  self->write_packed_csr(PepCSRBank::pack(n, z, v, c));
}

void handle_cpwr(PepISA3CPU *self, Op op, u16 op_val) {
  const isa::Pep10::Register reg = static_cast<isa::Pep10::Register>(op.target);
  const u16 src = self->read_register(reg);
  // Perform bitwise negation at u16 to match how the underlying HW adder works.
  // Perform arithmetic at u32 so that we can test for carry out directly rather than using multiple < comparisons.
  const u32 tmp_ext = 1_u32 + u32(src) + u16(~op_val);
  const u16 tmp = tmp_ext;
  const u16 neg = ~op_val + 1;
  // Is negative if high order bit is 1.
  bool n = tmp & 0x8000;
  // Is zero if all bits are 0's.
  bool z = tmp == 0x0000;
  // There is a signed overflow iff the high order bits of the register
  // and operand are the same, and one input & the output differ in sign.
  // >> Shifts in 0's (unsigned shorts), so after shift, only high order
  // bit remain.
  bool v = (~(src ^ neg) & (src ^ tmp)) >> 15;
  // Carry out iff tmp[16] == 1
  bool c = tmp_ext & 0x1'0000;
  // Invert N bit if there was signed overflow.
  n ^= v;
  self->write_packed_csr(PepCSRBank::pack(n, z, v, c));
}

void handle_cpbr(PepISA3CPU *self, Op op, u8 op_val) {
  const isa::Pep10::Register reg = static_cast<isa::Pep10::Register>(op.target);
  const auto src = self->read_register(reg);
  // The result is the decoded operand specifier plus A/X. mask down to a byte.
  u16 tmp = (src + ~op_val + 1) & 0xff;
  // Is negative if high order bit is 1.
  bool n = tmp & 0x80;
  // Is zero if all bits are 0's.
  bool z = tmp == 0x00;
  // RTL specifies that VC get 0.
  self->write_packed_csr(PepCSRBank::pack(n, z, 0, 0));
}

void handle_ldwr(PepISA3CPU *self, Op op, u16 op_val) {
  const isa::Pep10::Register reg = static_cast<isa::Pep10::Register>(op.target);
  auto [n, z, v, c] = PepCSRBank::unpack(self->read_packed_csr());
  // Is negative if high order bit is 1.
  n = op_val & 0x8000;
  z = op_val == 0x0000;

  self->write_register(reg, op_val);
  self->write_packed_csr(PepCSRBank::pack(n, z, v, c));
}

void handle_ldbr(PepISA3CPU *self, Op op, u8 op_val) {
  const isa::Pep10::Register reg = static_cast<isa::Pep10::Register>(op.target);
  auto [n, z, v, c] = PepCSRBank::unpack(self->read_packed_csr());
  // LDBr always clears n.
  n = 0;
  z = op_val == 0x0000;

  self->write_register(reg, op_val);
  self->write_packed_csr(PepCSRBank::pack(n, z, v, c));
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
