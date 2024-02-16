/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "cpu.hpp"
#include "bits/operations/swap.hpp"
#include "targets/pep10/isa3/helpers.hpp"
#include <bit>

sim::api2::memory::Operation rw_d = {
    .type = sim::api2::memory::Operation::Type::Standard,
    .kind = sim::api2::memory::Operation::Kind::data,
};

sim::api2::memory::Operation rw_i = {
    .type = sim::api2::memory::Operation::Type::Standard,
    .kind = sim::api2::memory::Operation::Kind::instruction,
};

targets::pep10::isa::CPU::CPU(sim::api2::device::Descriptor device,
                              sim::api2::device::IDGenerator gen)
    : _device(device),
      _regs({.id = gen(),
             .baseName = "regs",
             .fullName = _device.fullName + "/regs"},
            {.minOffset = 0,
             .maxOffset = quint8(::isa::Pep10::RegisterCount * 2 - 1)}),
      _csrs({.id = gen(),
             .baseName = "csrs",
             .fullName = _device.fullName + "/csrs"},
            {.minOffset = 0, .maxOffset = quint8(::isa::Pep10::CSRCount - 1)}) {
}

sim::api2::memory::Target<quint8> *targets::pep10::isa::CPU::regs() {
  return &_regs;
}

sim::api2::memory::Target<quint8> *targets::pep10::isa::CPU::csrs() {
  return &_csrs;
}

targets::pep10::isa::CPU::Status targets::pep10::isa::CPU::status() const {
  return _status;
}

const sim::api2::tick::Source *targets::pep10::isa::CPU::getSource() {
  return _clock;
}

void targets::pep10::isa::CPU::setSource(sim::api2::tick::Source *clock) {
    _clock = clock;
}

sim::api2::tick::Result targets::pep10::isa::CPU::clock(sim::api2::tick::Type currentTick)
{
  using Register = ::isa::Pep10::Register;
  quint16 pc = readReg(Register::PC);

  // Instruction specifier fetch + writeback.
  quint8 is = 0;
  _memory->read(pc, {&is, 1}, rw_i);
  writeReg(Register::IS, is);
  pc += 1;

  sim::api2::tick::Result ret;
  if (::isa::Pep10::isOpcodeUnary(is)) {
    // Increment PC and writeback
    writeReg(Register::PC, pc);
    // Execute unary dispatch
    ret = unaryDispatch(is);
  } else {
    // Instruction specifier fetch + writeback.
    quint16 os = 0;
    _memory->read(pc, {reinterpret_cast<quint8 *>(&os), 2}, rw_i);
    os = bits::hostOrder() != bits::Order::BigEndian ? bits::byteswap(os) : os;
    writeReg(Register::OS, os);
    // Execute nonunary dispatch, which is responsible for writing back PC.
    ret = nonunaryDispatch(is, os, pc += 2);
  }
  // TODO: Check for BP's
  ret.pause = false;
  return ret;
}

bool targets::pep10::isa::CPU::analyze(sim::api2::trace::PacketIterator iter, Direction)
{
  // At the moment, this class does not emit any trace events directly.
  return false;
}

void targets::pep10::isa::CPU::setBuffer(sim::api2::trace::Buffer *tb) {
  _tb = tb;
  _regs.setBuffer(tb);
  _csrs.setBuffer(tb);
}

void targets::pep10::isa::CPU::trace(bool enabled) {
  if (_tb)
    _tb->trace(_device.id, enabled);
  _regs.trace(enabled);
  _csrs.trace(enabled);
}

void targets::pep10::isa::CPU::setTarget(
    sim::api2::memory::Target<quint16> *target, void* port) {
  _memory = target;
}

quint16 targets::pep10::isa::CPU::readReg(::isa::Pep10::Register reg) {
  quint16 ret = 0;
  isa::readRegister<quint8>(&_regs, reg, ret, rw_d);
  return ret;
}

void targets::pep10::isa::CPU::writeReg(::isa::Pep10::Register reg,
                                        quint16 val) {
  isa::writeRegister<quint8>(&_regs, reg, val, rw_d);
}

bool targets::pep10::isa::CPU::readCSR(::isa::Pep10::CSR csr) {
  bool ret = 0;
  isa::readCSR(&_csrs, csr, ret, rw_d);
  return ret;
}

void targets::pep10::isa::CPU::writeCSR(::isa::Pep10::CSR csr, bool val) {
  isa::writeCSR<quint8>(&_csrs, csr, val, rw_d);
}

quint8 targets::pep10::isa::CPU::readPackedCSR() {
  quint8 ret = 0;
  isa::readPackedCSR(&_csrs, ret, rw_d);
  return ret;
}

void targets::pep10::isa::CPU::writePackedCSR(quint8 val) {
  isa::writePackedCSR(&_csrs, val, rw_d);
}

sim::api2::tick::Result targets::pep10::isa::CPU::unaryDispatch(quint8 is) {
  using mn = ::isa::Pep10::Mnemonic;
  using Register = ::isa::Pep10::Register;

  static const bool swap = bits::hostOrder() != bits::Order::BigEndian;
  auto mnemonic = ::isa::Pep10::opcodeLUT[is];
  quint16 a = readReg(Register::A), sp = readReg(Register::SP),
          x = readReg(Register::X);
  quint16 tmp = 0;
  bits::span<const quint8> tmpSpan = {reinterpret_cast<const quint8 *>(&tmp),
                                      sizeof(tmp)};
  quint8 tmp8 = 0;
  // Long enough to either hold all regs or one ctx switch block.
  static constexpr quint8 registersBytes = 2 * ::isa::Pep10::RegisterCount;
  quint8 ctx[std::max<std::size_t>(registersBytes, 10)];
  auto ctxSpan = bits::span<quint8>{ctx, sizeof(ctx)};
  auto [n, z, v, c] = unpackCSR(readPackedCSR());

  switch (mnemonic.instr.mnemon) {
  case mn::RET:
    _memory->read(sp, {reinterpret_cast<quint8 *>(&tmp), 2}, rw_d);
    // Must byteswap tmp if on big endian host, as _memory stores in little
    // endian
    if (swap)
      tmp = bits::byteswap(tmp);
    writeReg(Register::PC, tmp);
    writeReg(Register::SP, sp + 2);
    break;

  case mn::MOVSPA:
    writeReg(Register::A, sp);
    break;
  case mn::MOVASP:
    writeReg(Register::SP, a);
    break;

  case mn::MOVFLGA:
    writeReg(Register::A, readPackedCSR());
    break;
  case mn::MOVAFLG:
    writePackedCSR(a);
    break;

  case mn::NOP:
    break;

  case mn::NOTA:
    tmp = ~a;
    n = tmp & 0x8000;
    z = tmp == 0x0000;
    writeReg(Register::A, tmp);
    writePackedCSR(packCSR(n, z, v, c));
    break;
  case mn::NOTX:
    tmp = ~x;
    n = tmp & 0x8000;
    z = tmp == 0x0000;
    writeReg(Register::X, tmp);
    writePackedCSR(packCSR(n, z, v, c));
    break;

  case mn::NEGA:
    tmp = ~a + 1;
    n = tmp & 0x8000;
    z = tmp == 0x0000;
    v = tmp == 0x8000;
    c = a == 0x0000;
    writeReg(Register::A, tmp);
    writePackedCSR(packCSR(n, z, v, c));
    break;
  case mn::NEGX:
    tmp = ~x + 1;
    n = tmp & 0x8000;
    z = tmp == 0x0000;
    v = tmp == 0x8000;
    c = x == 0x0000;
    writeReg(Register::X, tmp);
    writePackedCSR(packCSR(n, z, v, c));
    break;

  case mn::ASLA:
    // Store in temp, because we need acc for status bit computation.
    tmp = a << 1;
    // Is negative if high order bit is 1.
    n = tmp & 0x8000;
    // Is zero if all bits are 0's.
    z = tmp == 0x0000;
    // Signed overflow occurs when the starting & ending values of the high
    // order bit differ (a xor temp == 1). Then shift the result over by 15
    // places to only keep high order bit (which is the sign).
    v = (a ^ tmp) >> 15;
    // Carry out if register starts with high order 1.
    c = a & 0x8000;
    writeReg(Register::A, tmp);
    writePackedCSR(packCSR(n, z, v, c));
    break;
  case mn::ASLX:
    // Store in temp, because we need acc for status bit computation.
    tmp = x << 1;
    // Is negative if high order bit is 1.
    n = tmp & 0x8000;
    // Is zero if all bits are 0's.
    z = tmp == 0x0000;
    // Signed overflow occurs when the starting & ending values of the high
    // order bit differ (a xor temp == 1). Then shift the result over by 15
    // places to only keep high order bit (which is the sign).
    v = (x ^ tmp) >> 15;
    // Carry out if register starts with high order 1.
    c = x & 0x8000;
    writeReg(Register::X, tmp);
    writePackedCSR(packCSR(n, z, v, c));
    break;

  case mn::ASRA:
    // Shift all bits to the right by 1 position. Since using unsigned shift,
    // must explicitly perform sign extension by hand.
    tmp = static_cast<quint16>(a >> 1 | ((a & 0x8000) ? 1 << 15 : 0));
    // Is negative if high order bit is 1.
    n = tmp & 0x8000;
    // Is zero if all bits are 0's.
    z = tmp == 0x000;
    // Carry out if register starts with low order 1.
    c = a & 0x1;
    v = 0;
    writeReg(Register::A, tmp);
    writePackedCSR(packCSR(n, z, v, c));
    break;
  case mn::ASRX:
    // Shift all bits to the right by 1 position. Since using unsigned shift,
    // must explicitly perform sign extension by hand.
    tmp = static_cast<quint16>(x >> 1 | ((x & 0x8000) ? 1 << 15 : 0));
    // Is negative if high order bit is 1.
    n = tmp & 0x8000;
    // Is zero if all bits are 0's.
    z = tmp == 0x000;
    // Carry out if register starts with low order 1.
    c = x & 0x1;
    v = 0;
    writeReg(Register::X, tmp);
    writePackedCSR(packCSR(n, z, v, c));
    break;

  case mn::ROLA:
    // Shift the carry in to low order bit.
    tmp = static_cast<quint16>(a << 1 | (c ? 1 : 0));
    // Carry out if register starts with high order 1.
    c = a & 0x8000;
    writeReg(Register::A, tmp);
    writePackedCSR(packCSR(n, z, v, c));
    break;
  case mn::ROLX:
    // Shift the carry in to low order bit.
    tmp = static_cast<quint16>(x << 1 | (c ? 1 : 0));
    // Carry out if register starts with high order 1.
    c = x & 0x8000;
    writeReg(Register::X, tmp);
    writePackedCSR(packCSR(n, z, v, c));
    break;

  case mn::RORA:
    // Shift the carry in to high order bit.
    tmp = a >> 1 | (c ? 1 << 15 : 0);
    // Carry out if register starts with low order 1.
    c = a & 0x1;
    writeReg(Register::A, tmp);
    writePackedCSR(packCSR(n, z, v, c));
    break;
  case mn::RORX:
    // Shift the carry in to high order bit.
    tmp = x >> 1 | (c ? 1 << 15 : 0);
    // Carry out if register starts with low order 1.
    c = x & 0x1;
    writeReg(Register::X, tmp);
    writePackedCSR(packCSR(n, z, v, c));
    break;

  case mn::SRET:
    // Fill ctx with all register's current values.
    // Then we can do a single write back to _regs and only generate 1 trace
    // packet.
    tmp = (_regs.span().maxOffset - _regs.span().minOffset + 1);
    _regs.read(0, {ctx, tmp}, rw_d);

    // Reload NZVC
    _memory->read(sp, {reinterpret_cast<quint8 *>(&tmp8), 1}, rw_d);
    writePackedCSR(tmp8);

    // Load A into ctx. No need for byteswap, _memory is little endian as are
    // regs.
    _memory->read(
        sp + 1, {ctx + 2 * static_cast<quint8>(Register::A), 2}, rw_d);
    swap ? bits::byteswap(tmp) : tmp;

    // Load X into ctx
    _memory->read(
        sp + 3, {ctx + 2 * static_cast<quint8>(Register::X), 2}, rw_d);

    // Load PC into ctx
    _memory->read(
        sp + 5, {ctx + 2 * static_cast<quint8>(Register::PC), 2}, rw_d);

    // Load SP into ctx
    _memory->read(
        sp + 7, {ctx + 2 * static_cast<quint8>(Register::SP), 2}, rw_d);

    // Bulk write-back regs, saving a number of bits on trace metadata.
    _regs.write(0, {ctx, registersBytes}, rw_d);

    tmp = sp + 10;
    // Using "host"'s variables, so byte swap if necessary.
    if (swap)
      tmp = bits::byteswap(tmp);
    _memory->write(
        static_cast<quint16>(::isa::Pep10::MemoryVectors::SystemStackPtr),
        {reinterpret_cast<quint8 *>(&tmp), 2}, rw_d);
    break;

  case mn::SCALL:
    // Must byteswap because we are using "host" variables.
    ctx[0] = readPackedCSR();
    tmp = swap ? bits::byteswap(a) : a;
    bits::memcpy(ctxSpan.subspan(1, 2), tmpSpan);
    tmp = swap ? bits::byteswap(x) : x;
    bits::memcpy(ctxSpan.subspan(3, 2), tmpSpan);
    tmp = readReg(Register::PC);
    tmp = swap ? bits::byteswap(tmp) : tmp;
    bits::memcpy(ctxSpan.subspan(5, 2), tmpSpan);
    tmp = swap ? bits::byteswap(sp) : sp;
    bits::memcpy(ctxSpan.subspan(7, 2), tmpSpan);
    ctx[9] = is;

    // Read system stack address.
    _memory->read(
        static_cast<quint16>(::isa::Pep10::MemoryVectors::SystemStackPtr),
        {reinterpret_cast<quint8 *>(&tmp), 2}, rw_d);
    if (swap)
      tmp = bits::byteswap(tmp);

    // Allocate ctx frame with -=.
    _memory->write(tmp -= 10, {ctx, 10}, rw_d);
    // And update SP with OS's SP.
    writeReg(Register::SP, tmp);

    // Read trap handler pc.
    _memory->read(
        static_cast<quint16>(::isa::Pep10::MemoryVectors::TrapHandler),
        {reinterpret_cast<quint8 *>(&tmp), 2}, rw_d);
    if (swap)
      tmp = bits::byteswap(tmp);
    writeReg(Register::PC, tmp);
    break;
  default:
      _status = Status::IllegalOpcode;
      throw std::logic_error("Illegal opcode");
  }
  return {.pause=0, .delay=1};
}

sim::api2::tick::Result
targets::pep10::isa::CPU::nonunaryDispatch(quint8 is, quint16 os, quint16 pc) {
  using mn = ::isa::Pep10::Mnemonic;
  using Register = ::isa::Pep10::Register;

  static const bool swap = bits::hostOrder() != bits::Order::BigEndian;
  auto mnemonic = ::isa::Pep10::opcodeLUT[is];
  quint16 a = readReg(Register::A), sp = readReg(Register::SP),
          x = readReg(Register::X);

  quint16 operand = 0;

  quint16 tmp = 0;
  auto [n, z, v, c] = unpackCSR(readPackedCSR());

  auto instrDef = ::isa::Pep10::opcodeLUT[is];
  if (::isa::Pep10::isValidAddressingMode(instrDef.instr.mnemon,
                                          instrDef.mode))
    ::isa::Pep10::isStore(is) ? decodeStoreOperand(is, os, operand)
                              : decodeLoadOperand(is, os, operand);
  else
      throw std::logic_error("Invalid addressing mode");


  switch (mnemonic.instr.mnemon) {
  case mn::BR:
    pc = operand;
    break;
  case mn::BRLE:
    pc = (n || z) ? operand : pc;
    break;
  case mn::BRLT:
    pc = n ? operand : pc;
    break;
  case mn::BREQ:
    pc = z ? operand : pc;
    break;
  case mn::BRNE:
    pc = !z ? operand : pc;
    break;
  case mn::BRGE:
    pc = !n ? operand : pc;
    break;
  case mn::BRGT:
    pc = (!n && !z) ? operand : pc;
    break;
  case mn::BRV:
    pc = v ? operand : pc;
    break;
  case mn::BRC:
    pc = c ? operand : pc;
    break;
  case mn::CALL:
    // Write PC to stack
    tmp = swap ? bits::byteswap(pc) : pc;
    _memory->write(sp -= 2, {reinterpret_cast<quint8 *>(&tmp), 2}, rw_d);
    pc = operand;
    writeReg(Register::SP, sp);
    break;

  case mn::LDWA:
    writeReg(Register::A, operand);
    n = operand & 0x8000;
    z = operand == 0x0000;
    writePackedCSR(packCSR(n, z, v, c));
    break;
  case mn::LDWX:
    writeReg(Register::X, operand);
    n = operand & 0x8000;
    z = operand == 0x0000;
    writePackedCSR(packCSR(n, z, v, c));
    break;

  // LDBx instructions depend on decodeLoadOperand to 0-fill upper byte.
  case mn::LDBA:
    writeReg(Register::A, operand);
    // LDBx always clears n.
    n = 0;
    z = operand == 0x0000;
    writePackedCSR(packCSR(n, z, v, c));
    break;
  case mn::LDBX:
    writeReg(Register::X, operand);
    // LDBx always clears n.
    n = 0;
    z = operand == 0x0000;
    writePackedCSR(packCSR(n, z, v, c));
    break;

  case mn::STWA:
    tmp = swap ? bits::byteswap(a) : a;
    _memory->write(operand, {reinterpret_cast<quint8 *>(&tmp), 2}, rw_d);
    break;
  case mn::STWX:
    tmp = swap ? bits::byteswap(x) : x;
    _memory->write(operand, {reinterpret_cast<quint8 *>(&tmp), 2}, rw_d);
    break;

  case mn::STBA:
    tmp = swap ? bits::byteswap(a) : a;
    _memory->write(operand, {reinterpret_cast<quint8 *>(&tmp) + 1, 1}, rw_d);
    break;
  case mn::STBX:
    tmp = swap ? bits::byteswap(x) : x;
    _memory->write(operand, {reinterpret_cast<quint8 *>(&tmp) + 1, 1}, rw_d);
    break;

  case mn::CPWA:
    tmp = a + ~operand + 1;
    // Is negative if high order bit is 1.
    n = tmp & 0x8000;
    // Is zero if all bits are 0's.
    z = tmp == 0x0000;
    // There is a signed overflow iff the high order bits of the register
    // and operand are the same, and one input & the output differ in sign.
    // >> Shifts in 0's (unsigned shorts), so after shift, only high order
    // bit remain.
    v = (~(a ^ (~operand + 1)) & (a ^ tmp)) >> 15;
    // Carry out iff result is unsigned less than register or operand.
    c = tmp < a || tmp < static_cast<quint16>(1 + ~operand);
    // Invert N bit if there was signed overflow.
    n ^= v;
    writePackedCSR(packCSR(n, z, v, c));
    break;
  case mn::CPWX:
    tmp = x + ~operand + 1;
    // Is negative if high order bit is 1.
    n = tmp & 0x8000;
    // Is zero if all bits are 0's.
    z = tmp == 0x0000;
    // There is a signed overflow iff the high order bits of the register
    // and operand are the same, and one input & the output differ in sign.
    // >> Shifts in 0's (unsigned shorts), so after shift, only high order
    // bit remain.
    v = (~(x ^ (~operand + 1)) & (x ^ tmp)) >> 15;
    // Carry out iff result is unsigned less than register or operand.
    c = tmp < x || tmp < static_cast<quint16>(1 + ~operand);
    // Invert N bit if there was signed overflow.
    n ^= v;
    writePackedCSR(packCSR(n, z, v, c));
    break;

  case mn::CPBA:
    // The result is the decoded operand specifier plus the accumulator
    tmp = (a + ~operand + 1) & 0xff;
    // Is negative if high order bit is 1.
    n = tmp & 0x80;
    // Is zero if all bits are 0's.
    z = tmp == 0x00;
    // RTL specifies that VC get 0.
    v = c = 0;
    writePackedCSR(packCSR(n, z, v, c));
    break;
  case mn::CPBX:
    // The result is the decoded operand specifier plus the accumulator
    tmp = (x + ~operand + 1) & 0xff;
    // Is negative if high order bit is 1.
    n = tmp & 0x80;
    // Is zero if all bits are 0's.
    z = tmp == 0x00;
    // RTL specifies that VC get 0.
    v = c = 0;
    writePackedCSR(packCSR(n, z, v, c));
    break;

  case mn::ADDA:
    // The result is the decoded operand specifier plus the accumulator
    tmp = a + operand;
    writeReg(Register::A, tmp);
    // Is negative if high order bit is 1.
    n = tmp & 0x8000;
    // Is zero if all bits are 0's.
    z = tmp == 0x0000;
    // There is a signed overflow iff the high order bits of the register
    // and operand are the same, and one input & the output differ in sign.
    // >> Shifts in 0's (unsigned shorts), so after shift, only high order
    // bit remain.
    v = (~(a ^ operand) & (a ^ tmp)) >> 15;
    // Carry out iff result is unsigned less than register or operand.
    c = tmp < a || tmp < operand;
    writePackedCSR(packCSR(n, z, v, c));
    break;
  case mn::ADDX:
    // The result is the decoded operand specifier plus the index register.
    tmp = x + operand;
    writeReg(Register::X, tmp);
    // Is negative if high order bit is 1.
    n = tmp & 0x8000;
    // Is zero if all bits are 0's.
    z = tmp == 0x0000;
    // There is a signed overflow iff the high order bits of the register
    // and operand are the same, and one input & the output differ in sign.
    // >> Shifts in 0's (unsigned shorts), so after shift, only high order
    // bit remain.
    v = (~(x ^ operand) & (x ^ tmp)) >> 15;
    // Carry out iff result is unsigned less than register or operand.
    c = tmp < x || tmp < operand;
    writePackedCSR(packCSR(n, z, v, c));
    break;

  case mn::SUBA:
    // The result is the negated decoded operand specifier plus the
    // accumulator
    tmp = a + ~operand + 1;
    writeReg(Register::A, tmp);
    // Is negative if high order bit is 1.
    n = tmp & 0x8000;
    // Is zero if all bits are 0's.
    z = tmp == 0x0000;
    // There is a signed overflow iff the high order bits of the register
    // and operand are the same, and one input & the output differ in sign.
    // >> Shifts in 0's (unsigned shorts), so after shift, only high order
    // bit remain.
    v = (~(a ^ (~operand + 1)) & (a ^ tmp)) >> 15;
    // Carry out iff result is unsigned less than register or operand.
    c = tmp < a || tmp < static_cast<quint16>(1 + ~operand);
    writePackedCSR(packCSR(n, z, v, c));
    break;
  case mn::SUBX:
    // The result is the negated decoded operand specifier plus the index
    // register
    tmp = x + ~operand + 1;
    writeReg(Register::X, tmp);
    // Is negative if high order bit is 1.
    n = tmp & 0x8000;
    // Is zero if all bits are 0's.
    z = tmp == 0x0000;
    // There is a signed overflow iff the high order bits of the register
    // and operand are the same, and one input & the output differ in sign.
    v = (~(x ^ (~operand + 1)) & (x ^ tmp)) >> 15;
    // Carry out iff result is unsigned less than register or operand.
    c = tmp < x || tmp < static_cast<quint16>(1 + ~operand);
    writePackedCSR(packCSR(n, z, v, c));
    break;

  case mn::ANDA:
    tmp = a & operand;
    writeReg(Register::A, tmp);
    // Is negative if high order bit is 1.
    n = tmp & 0x8000;
    // Is zero if all bits are 0's.
    z = tmp == 0x0000;
    writePackedCSR(packCSR(n, z, v, c));
    break;
  case mn::ANDX:
    tmp = x & operand;
    writeReg(Register::X, tmp);
    // Is negative if high order bit is 1.
    n = tmp & 0x8000;
    // Is zero if all bits are 0's.
    z = tmp == 0x0000;
    writePackedCSR(packCSR(n, z, v, c));
    break;

  case mn::ORA:
    tmp = a | operand;
    writeReg(Register::A, tmp);
    // Is negative if high order bit is 1.
    n = tmp & 0x8000;
    // Is zero if all bits are 0's.
    z = tmp == 0x0000;
    writePackedCSR(packCSR(n, z, v, c));
    break;
  case mn::ORX:
    tmp = x | operand;
    writeReg(Register::X, tmp);
    // Is negative if high order bit is 1.
    n = tmp & 0x8000;
    // Is zero if all bits are 0's.
    z = tmp == 0x0000;
    writePackedCSR(packCSR(n, z, v, c));
    break;

  case mn::XORA:
    tmp = a ^ operand;
    writeReg(Register::A, tmp);
    // Is negative if high order bit is 1.
    n = tmp & 0x8000;
    // Is zero if all bits are 0's.
    z = tmp == 0x0000;
    writePackedCSR(packCSR(n, z, v, c));
    break;
  case mn::XORX:
    tmp = x ^ operand;
    writeReg(Register::X, tmp);
    // Is negative if high order bit is 1.
    n = tmp & 0x8000;
    // Is zero if all bits are 0's.
    z = tmp == 0x0000;
    writePackedCSR(packCSR(n, z, v, c));
    break;

  case mn::ADDSP:
    writeReg(Register::SP, sp + operand);
    break;
  case mn::SUBSP:
    writeReg(Register::SP, sp - operand);
    break;
  default:
      _status = Status::IllegalOpcode;
      throw std::runtime_error("Illegal Opcode");
  }

  // Increment PC and writeback
  writeReg(Register::PC, pc);
  return {.pause=0, .delay=1};
}

void
targets::pep10::isa::CPU::decodeStoreOperand(quint8 is, quint16 os,
                                             quint16 &decoded) {
  using Register = ::isa::Pep10::Register;
  using am = ::isa::Pep10::AddressingMode;

  static const bool swap = bits::hostOrder() != bits::Order::BigEndian;
  auto instruction = ::isa::Pep10::opcodeLUT[is];

  switch (instruction.mode) {
  // case am::I:
  case am::D:
    decoded = os;
    break;
  case am::N:
    _memory->read(os, {reinterpret_cast<quint8 *>(&decoded), 2}, rw_i);
    if (swap)
      decoded = bits::byteswap(decoded);
    break;
  case am::S:
    decoded = os + readReg(Register::SP);
    break;
  case am::X:
    decoded = os + readReg(Register::X);
    break;
  case am::SX:
    decoded = os + readReg(Register::SP) + readReg(Register::X);
    break;
  case am::SF:
    _memory->read(os + readReg(Register::SP),
                  {reinterpret_cast<quint8 *>(&decoded), 2}, rw_i);
    if (swap)
      decoded = bits::byteswap(decoded);
    break;
  case am::SFX:
    _memory->read(os + readReg(Register::SP),
                  {reinterpret_cast<quint8 *>(&decoded), 2}, rw_i);
    if (swap)
      decoded = bits::byteswap(decoded);
    decoded += readReg(Register::X);
    break;
  default:
    throw std::logic_error("Invalid addressing mode");
  }
}

void
targets::pep10::isa::CPU::decodeLoadOperand(quint8 is, quint16 os,
                                            quint16 &decoded) {
  using Register = ::isa::Pep10::Register;
  using mn = ::isa::Pep10::Mnemonic;
  using am = ::isa::Pep10::AddressingMode;

  static const bool swap = bits::hostOrder() != bits::Order::BigEndian;
  auto instruction = ::isa::Pep10::opcodeLUT[is];
  auto mnemon = instruction.instr.mnemon;
  bool isByte = mnemon == mn::LDBA || mnemon == mn::LDBX ||
                mnemon == mn::CPBA || mnemon == mn::CPBX;
  quint16 mask = isByte ? 0xFF : 0xFFFF;
  switch (instruction.mode) {
  case am::I:
    decoded = os;
    break;
  case am::D:
    _memory->read(os,
        {reinterpret_cast<quint8 *>(&decoded) + int(isByte && swap ? 1 : 0),
         std::size_t(isByte ? 1 : 2)}, rw_i);
    if (swap)
      decoded = bits::byteswap(decoded);
    break;
  case am::N:
    _memory->read(os, {reinterpret_cast<quint8 *>(&decoded), 2}, rw_i);

    if (swap)
      decoded = bits::byteswap(decoded);
    _memory->read(decoded,
        {reinterpret_cast<quint8 *>(&decoded) + int(isByte && swap ? 1 : 0),
         std::size_t(isByte ? 1 : 2)}, rw_i);
    if (swap)
      decoded = bits::byteswap(decoded);
    break;
  case am::S:
    _memory->read(os + readReg(Register::SP),
        {reinterpret_cast<quint8 *>(&decoded) + int(isByte && swap ? 1 : 0),
         std::size_t(isByte ? 1 : 2)}, rw_i);
    if (swap)
      decoded = bits::byteswap(decoded);
    break;
  case am::X:
    _memory->read(os + readReg(Register::X),
        {reinterpret_cast<quint8 *>(&decoded) + int(isByte && swap ? 1 : 0),
         std::size_t(isByte ? 1 : 2)}, rw_i);
    if (swap)
      decoded = bits::byteswap(decoded);
    break;
  case am::SX:
    _memory->read(os + readReg(Register::SP) + readReg(Register::X),
        {reinterpret_cast<quint8 *>(&decoded) + int(isByte && swap ? 1 : 0),
         std::size_t(isByte ? 1 : 2)}, rw_i);
    if (swap)
      decoded = bits::byteswap(decoded);
    break;
  case am::SF:
    _memory->read(os + readReg(Register::SP),
        {reinterpret_cast<quint8 *>(&decoded), 2}, rw_i);

    if (swap)
      decoded = bits::byteswap(decoded);
    _memory->read(decoded,
        {reinterpret_cast<quint8 *>(&decoded) + int(isByte && swap ? 1 : 0),
         std::size_t(isByte ? 1 : 2)}, rw_i);
    if (swap)
      decoded = bits::byteswap(decoded);
    break;
  case am::SFX:
    _memory->read(os + readReg(Register::SP),
        {reinterpret_cast<quint8 *>(&decoded), 2}, rw_i);

    if (swap)
      decoded = bits::byteswap(decoded);
    _memory->read(decoded + readReg(Register::X),
        {reinterpret_cast<quint8 *>(&decoded) + int(isByte && swap ? 1 : 0),
         std::size_t(isByte ? 1 : 2)}, rw_i);
    if (swap)
      decoded = bits::byteswap(decoded);
    break;
  default:
    throw std::logic_error("Invalid addressing mode");
  }
  decoded &= mask;
}
