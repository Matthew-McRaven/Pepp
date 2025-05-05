/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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
#include "sim/device/broadcast/mmo.hpp"
#include "targets/isa3/helpers.hpp"
#include "utils/bits/swap.hpp"

namespace {
sim::api2::memory::Operation rw_d = {
    .type = sim::api2::memory::Operation::Type::Standard,
    .kind = sim::api2::memory::Operation::Kind::data,
};

sim::api2::memory::Operation rw_i = {
    .type = sim::api2::memory::Operation::Type::Standard,
    .kind = sim::api2::memory::Operation::Kind::instruction,
};

sim::api2::memory::Operation gs_i = {
    .type = sim::api2::memory::Operation::Type::Application,
    .kind = sim::api2::memory::Operation::Kind::instruction,
};
} // namespace
template <typename T> using AddressSpan = sim::api2::memory::AddressSpan<T>;
targets::pep9::isa::CPU::CPU(sim::api2::device::Descriptor device, sim::api2::device::IDGenerator gen)
    : _device(device), _regs({.id = gen(), .baseName = "regs", .fullName = _device.fullName + "/regs"},
                             AddressSpan<quint8>(0, quint8(::isa::Pep9::RegisterCount * 2 - 1))),
      _csrs({.id = gen(), .baseName = "csrs", .fullName = _device.fullName + "/csrs"},
            AddressSpan<quint8>(0, quint8(::isa::Pep9::CSRCount - 1))) {}

sim::api2::memory::Target<quint8> *targets::pep9::isa::CPU::regs() { return &_regs; }

sim::api2::memory::Target<quint8> *targets::pep9::isa::CPU::csrs() { return &_csrs; }

sim::api2::device::Descriptor targets::pep9::isa::CPU::device() const { return _device; }

targets::pep9::isa::CPU::Status targets::pep9::isa::CPU::status() const { return _status; }

void targets::pep9::isa::CPU::setPwrOff(sim::memory::Output<quint16> *pwrOff) { _pwrOff = pwrOff; }

std::optional<quint16> targets::pep9::isa::CPU::currentOperand() {
  using Register = ::isa::Pep9::Register;
  auto is = readReg(Register::IS), os = readReg(Register::OS);
  auto instrDef = ::isa::Pep9::opcodeLUT[is];
  if (::isa::Pep9::isValidAddressingMode(instrDef.instr.mnemon, instrDef.mode) && !instrDef.instr.unary) {
    quint16 operand = 0;
    ::isa::Pep9::isStore(is) ? decodeStoreOperand(is, os, operand, false) : decodeLoadOperand(is, os, operand, false);
    return operand;
  }
  return std::nullopt;
}

quint16 targets::pep9::isa::CPU::startingPC() const { return _startingPC; }

void targets::pep9::isa::CPU::updateStartingPC() { _startingPC = readReg(::isa::Pep9::Register::PC); }

quint16 targets::pep9::isa::CPU::depth() const { return _depth; }

const sim::api2::tick::Source *targets::pep9::isa::CPU::getSource() { return _clock; }

void targets::pep9::isa::CPU::setSource(sim::api2::tick::Source *clock) { _clock = clock; }

sim::api2::tick::Result targets::pep9::isa::CPU::clock(sim::api2::tick::Type currentTick) {
  using Register = ::isa::Pep9::Register;
  quint16 pc = _startingPC = readReg(Register::PC);

  // Instruction specifier fetch + writeback.
  quint8 is = 0;
  _memory->read(pc, {&is, 1}, rw_i);
  writeReg(Register::IS, is);
  pc += 1;

  sim::api2::tick::Result ret;
  if (::isa::Pep9::isOpcodeUnary(is)) {
    // Execute unary dispatch
    ret = unaryDispatch(is, pc);
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

bool targets::pep9::isa::CPU::analyze(sim::api2::trace::PacketIterator iter, sim::api2::trace::Direction) {
  // TODO: handle increment packets for depth value.
  // At the moment, this class does not emit any trace events directly.
  return false;
}

void targets::pep9::isa::CPU::setBuffer(sim::api2::trace::Buffer *tb) {
  _tb = tb;
  _regs.setBuffer(tb);
  _csrs.setBuffer(tb);
}

void targets::pep9::isa::CPU::trace(bool enabled) {
  if (_tb) _tb->trace(_device.id, enabled);
  _regs.trace(enabled);
  _csrs.trace(enabled);
}

void targets::pep9::isa::CPU::setTarget(sim::api2::memory::Target<quint16> *target, void *port) { _memory = target; }

void targets::pep9::isa::CPU::setDebugger(pepp::debug::Debugger *debugger) { _dbg = debugger; }

void targets::pep9::isa::CPU::clearDebugger() { _dbg = nullptr; }

void targets::pep9::isa::CPU::incrDepth() {
  static const quint8 amt = 1;
  _depth++;
  if (_tb) _tb->emitIncrement<quint8>(_device.id, 0, {&amt, 1});
}

void targets::pep9::isa::CPU::decrDepth() {
  static const quint8 amt = -1;
  if (_depth != 0) _depth--;
  if (_tb) _tb->emitIncrement<quint8>(_device.id, 0, {&amt, 1});
}

quint16 targets::pep9::isa::CPU::readReg(::isa::Pep9::Register reg) {
  quint16 ret = 0;
  targets::isa::readRegister<::isa::Pep9, quint8>(&_regs, reg, ret, rw_d);
  return ret;
}

void targets::pep9::isa::CPU::writeReg(::isa::Pep9::Register reg, quint16 val) {
  targets::isa::writeRegister<::isa::Pep9, quint8>(&_regs, reg, val, rw_d);
}

bool targets::pep9::isa::CPU::readCSR(::isa::Pep9::CSR csr) {
  bool ret = 0;
  targets::isa::readCSR<::isa::Pep9>(&_csrs, csr, ret, rw_d);
  return ret;
}

void targets::pep9::isa::CPU::writeCSR(::isa::Pep9::CSR csr, bool val) {
  targets::isa::writeCSR<::isa::Pep9, quint8>(&_csrs, csr, val, rw_d);
}

quint8 targets::pep9::isa::CPU::readPackedCSR() {
  quint8 ret = 0;
  targets::isa::readPackedCSR<::isa::Pep9>(&_csrs, ret, rw_d);
  return ret;
}

void targets::pep9::isa::CPU::writePackedCSR(quint8 val) {
  targets::isa::writePackedCSR<::isa::Pep9>(&_csrs, val, rw_d);
}

sim::api2::tick::Result targets::pep9::isa::CPU::unaryDispatch(quint8 is, quint16 pc) {
  using ISA = ::isa::Pep9;
  using mn = ISA::Mnemonic;
  using Register = ISA::Pep9::Register;

  static const bool swap = bits::hostOrder() != bits::Order::BigEndian;
  auto mnemonic = ::isa::Pep9::opcodeLUT[is];
  quint16 a = readReg(Register::A), sp = readReg(Register::SP), x = readReg(Register::X);
  quint16 tmp = 0;
  bits::span<const quint8> tmpSpan = {reinterpret_cast<const quint8 *>(&tmp), sizeof(tmp)};
  quint8 tmp8 = 0;
  // Long enough to either hold all regs or one ctx switch block.
  static constexpr quint8 registersBytes = 2 * ::isa::Pep9::RegisterCount;
  quint8 ctx[std::max<std::size_t>(registersBytes, 10)];
  auto ctxSpan = bits::span<quint8>{ctx, sizeof(ctx)};
  auto [n, z, v, c] = targets::isa::unpackCSR<ISA>(readPackedCSR());

  switch (mnemonic.instr.mnemon) {
  case mn::STOP:
    tmp8 = 0xde;
    if (_pwrOff) _pwrOff->write(0, {&tmp8, 1}, rw_d);
    break;

  case mn::RET:
    _memory->read(sp, {reinterpret_cast<quint8 *>(&tmp), 2}, rw_d);
    // Must byteswap tmp if on big endian host, as _memory stores in little
    // endian
    if (swap) tmp = bits::byteswap(tmp);
    pc = tmp;
    writeReg(Register::SP, sp + 2);
    decrDepth();
    break;
  case mn::RETTR:
    // Fill ctx with all register's current values.
    // Then we can do a single write back to _regs and only generate 1 trace
    // packet.
    tmp = size_inclusive(_regs.span());
    _regs.read(0, {ctx, tmp}, rw_d);

    // Reload NZVC
    _memory->read(sp, {reinterpret_cast<quint8 *>(&tmp8), 1}, rw_d);
    writePackedCSR(tmp8);

    // Load A into ctx. No need for byteswap, _memory is little endian as are
    // regs.
    _memory->read(sp + 1, {ctx + 2 * static_cast<quint8>(Register::A), 2}, rw_d);
    swap ? bits::byteswap(tmp) : tmp;

    // Load X into ctx
    _memory->read(sp + 3, {ctx + 2 * static_cast<quint8>(Register::X), 2}, rw_d);

    // Load PC into ctx
    _memory->read(sp + 5, {ctx + 2 * static_cast<quint8>(Register::PC), 2}, rw_d);

    // Load SP into ctx
    _memory->read(sp + 7, {ctx + 2 * static_cast<quint8>(Register::SP), 2}, rw_d);

    // Bulk write-back regs, saving a number of bits on trace metadata.
    _regs.write(0, {ctx, registersBytes}, rw_d);

    tmp = sp + 10;
    // Using "host"'s variables, so byte swap if necessary.
    if (swap) tmp = bits::byteswap(tmp);
    _memory->write(static_cast<quint16>(::isa::Pep9::MemoryVectors::SystemStackPtr),
                   {reinterpret_cast<quint8 *>(&tmp), 2}, rw_d);
    if (_dbg) _dbg->bps->notifyPCChanged(readReg(Register::PC));
    decrDepth();
    return {.pause = 0, .delay = 1};

  case mn::MOVSPA: writeReg(Register::A, sp); break;

  case mn::MOVFLGA: writeReg(Register::A, readPackedCSR()); break;
  case mn::MOVAFLG: writePackedCSR(a); break;


  case mn::NOTA:
    tmp = ~a;
    n = tmp & 0x8000;
    z = tmp == 0x0000;
    writeReg(Register::A, tmp);
    writePackedCSR(targets::isa::packCSR<ISA>(n, z, v, c));
    break;
  case mn::NOTX:
    tmp = ~x;
    n = tmp & 0x8000;
    z = tmp == 0x0000;
    writeReg(Register::X, tmp);
    writePackedCSR(targets::isa::packCSR<ISA>(n, z, v, c));
    break;

  case mn::NEGA:
    tmp = ~a + 1;
    n = tmp & 0x8000;
    z = tmp == 0x0000;
    v = tmp == 0x8000;
    c = a == 0x0000;
    writeReg(Register::A, tmp);
    writePackedCSR(targets::isa::packCSR<ISA>(n, z, v, c));
    break;
  case mn::NEGX:
    tmp = ~x + 1;
    n = tmp & 0x8000;
    z = tmp == 0x0000;
    v = tmp == 0x8000;
    c = x == 0x0000;
    writeReg(Register::X, tmp);
    writePackedCSR(targets::isa::packCSR<ISA>(n, z, v, c));
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
    writePackedCSR(targets::isa::packCSR<ISA>(n, z, v, c));
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
    writePackedCSR(targets::isa::packCSR<ISA>(n, z, v, c));
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
    writePackedCSR(targets::isa::packCSR<ISA>(n, z, v, c));
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
    writePackedCSR(targets::isa::packCSR<ISA>(n, z, v, c));
    break;

  case mn::ROLA:
    // Shift the carry in to low order bit.
    tmp = static_cast<quint16>(a << 1 | (c ? 1 : 0));
    // Carry out if register starts with high order 1.
    c = a & 0x8000;
    writeReg(Register::A, tmp);
    writePackedCSR(targets::isa::packCSR<ISA>(n, z, v, c));
    break;
  case mn::ROLX:
    // Shift the carry in to low order bit.
    tmp = static_cast<quint16>(x << 1 | (c ? 1 : 0));
    // Carry out if register starts with high order 1.
    c = x & 0x8000;
    writeReg(Register::X, tmp);
    writePackedCSR(targets::isa::packCSR<ISA>(n, z, v, c));
    break;

  case mn::RORA:
    // Shift the carry in to high order bit.
    tmp = a >> 1 | (c ? 1 << 15 : 0);
    // Carry out if register starts with low order 1.
    c = a & 0x1;
    writeReg(Register::A, tmp);
    writePackedCSR(targets::isa::packCSR<ISA>(n, z, v, c));
    break;
  case mn::RORX:
    // Shift the carry in to high order bit.
    tmp = x >> 1 | (c ? 1 << 15 : 0);
    // Carry out if register starts with low order 1.
    c = x & 0x1;
    writeReg(Register::X, tmp);
    writePackedCSR(targets::isa::packCSR<ISA>(n, z, v, c));
    break;
  // Non-unary traps
  case mn::NOP: [[fallthrough]];
  case mn::DECI: [[fallthrough]];
  case mn::DECO: [[fallthrough]];
  case mn::HEXO: [[fallthrough]];
  case mn::STRO:
    // Though not part of the specification, the Pep9 hardware must increment the program counter
    // in order for non-unary traps to function correctly.
    pc += 2;
    [[fallthrough]];
  // Unary traps.
  case mn::NOP0: [[fallthrough]];
  case mn::NOP1:
    // Must byteswap because we are using "host" variables.
    ctx[0] = readPackedCSR();
    tmp = swap ? bits::byteswap(a) : a;
    bits::memcpy(ctxSpan.subspan(1, 2), tmpSpan);
    tmp = swap ? bits::byteswap(x) : x;
    bits::memcpy(ctxSpan.subspan(3, 2), tmpSpan);
    tmp = swap ? bits::byteswap(pc) : pc;
    bits::memcpy(ctxSpan.subspan(5, 2), tmpSpan);
    tmp = swap ? bits::byteswap(sp) : sp;
    bits::memcpy(ctxSpan.subspan(7, 2), tmpSpan);
    ctx[9] = is;

    // Read system stack address.
    _memory->read(static_cast<quint16>(::isa::Pep9::MemoryVectors::SystemStackPtr),
                  {reinterpret_cast<quint8 *>(&tmp), 2}, rw_d);
    if (swap) tmp = bits::byteswap(tmp);

    // Allocate ctx frame with -=.
    _memory->write(tmp -= 10, {ctx, 10}, rw_d);
    // And update SP with OS's SP.
    writeReg(Register::SP, tmp);

    // Read trap handler pc.
    _memory->read(static_cast<quint16>(::isa::Pep9::MemoryVectors::TrapHandler), {reinterpret_cast<quint8 *>(&tmp), 2},
                  rw_d);
    if (swap) tmp = bits::byteswap(tmp);
    pc = tmp;
    // Though not part of the specification, clear out the index register to
    // prevent bug in OS where non-unary instructions fail due to junk
    // in the high order byte of the index register. The book is published,
    // so we have to fix it here.
    writeReg(Register::X, 0);
    incrDepth();
    break;
  default:
    writeReg(Register::PC, pc);
    _status = Status::IllegalOpcode;
    static const char *const e = "Illegal opcode";
    qCritical(e);
    throw std::logic_error(e);
  }
  writeReg(Register::PC, pc);
  if (_dbg) _dbg->bps->notifyPCChanged(pc);
  return {.pause = 0, .delay = 1};
}

sim::api2::tick::Result targets::pep9::isa::CPU::nonunaryDispatch(quint8 is, quint16 os, quint16 pc) {
  using ISA = ::isa::Pep9;
  using mn = ISA::Mnemonic;
  using Register = ISA::Register;

  static const bool swap = bits::hostOrder() != bits::Order::BigEndian;
  auto mnemonic = ::isa::Pep9::opcodeLUT[is];
  quint16 a = readReg(Register::A), sp = readReg(Register::SP), x = readReg(Register::X), temp = 0;

  quint16 operand = 0;

  quint16 tmp = 0;
  auto [n, z, v, c] = targets::isa::unpackCSR<ISA>(readPackedCSR());

  auto instrDef = ::isa::Pep9::opcodeLUT[is];
  if (::isa::Pep9::isValidAddressingMode(instrDef.instr.mnemon, instrDef.mode))
    ::isa::Pep9::isStore(is) ? decodeStoreOperand(is, os, operand) : decodeLoadOperand(is, os, operand);
  else throw std::logic_error("Invalid addressing mode");

  switch (mnemonic.instr.mnemon) {
  case mn::BR: pc = operand; break;
  case mn::BRLE: pc = (n || z) ? operand : pc; break;
  case mn::BRLT: pc = n ? operand : pc; break;
  case mn::BREQ: pc = z ? operand : pc; break;
  case mn::BRNE: pc = !z ? operand : pc; break;
  case mn::BRGE: pc = !n ? operand : pc; break;
  case mn::BRGT: pc = (!n && !z) ? operand : pc; break;
  case mn::BRV: pc = v ? operand : pc; break;
  case mn::BRC: pc = c ? operand : pc; break;
  case mn::CALL:
    // Write PC to stack
    tmp = swap ? bits::byteswap(pc) : pc;
    _memory->write(sp -= 2, {reinterpret_cast<quint8 *>(&tmp), 2}, rw_d);
    pc = operand;
    writeReg(Register::SP, sp);
    incrDepth();
    break;

  case mn::ADDSP: writeReg(Register::SP, sp + operand); break;
  case mn::SUBSP: writeReg(Register::SP, sp - operand); break;

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
    writePackedCSR(targets::isa::packCSR<ISA>(n, z, v, c));
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
    writePackedCSR(targets::isa::packCSR<ISA>(n, z, v, c));
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
    writePackedCSR(targets::isa::packCSR<ISA>(n, z, v, c));
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
    writePackedCSR(targets::isa::packCSR<ISA>(n, z, v, c));
    break;

  case mn::ANDA:
    tmp = a & operand;
    writeReg(Register::A, tmp);
    // Is negative if high order bit is 1.
    n = tmp & 0x8000;
    // Is zero if all bits are 0's.
    z = tmp == 0x0000;
    writePackedCSR(targets::isa::packCSR<ISA>(n, z, v, c));
    break;
  case mn::ANDX:
    tmp = x & operand;
    writeReg(Register::X, tmp);
    // Is negative if high order bit is 1.
    n = tmp & 0x8000;
    // Is zero if all bits are 0's.
    z = tmp == 0x0000;
    writePackedCSR(targets::isa::packCSR<ISA>(n, z, v, c));
    break;

  case mn::ORA:
    tmp = a | operand;
    writeReg(Register::A, tmp);
    // Is negative if high order bit is 1.
    n = tmp & 0x8000;
    // Is zero if all bits are 0's.
    z = tmp == 0x0000;
    writePackedCSR(targets::isa::packCSR<ISA>(n, z, v, c));
    break;
  case mn::ORX:
    tmp = x | operand;
    writeReg(Register::X, tmp);
    // Is negative if high order bit is 1.
    n = tmp & 0x8000;
    // Is zero if all bits are 0's.
    z = tmp == 0x0000;
    writePackedCSR(targets::isa::packCSR<ISA>(n, z, v, c));
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
    writePackedCSR(targets::isa::packCSR<ISA>(n, z, v, c));
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
    writePackedCSR(targets::isa::packCSR<ISA>(n, z, v, c));
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
    writePackedCSR(targets::isa::packCSR<ISA>(n, z, v, c));
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
    writePackedCSR(targets::isa::packCSR<ISA>(n, z, v, c));
    break;

  case mn::LDWA:
    writeReg(Register::A, operand);
    n = operand & 0x8000;
    z = operand == 0x0000;
    writePackedCSR(targets::isa::packCSR<ISA>(n, z, v, c));
    break;
  case mn::LDWX:
    writeReg(Register::X, operand);
    n = operand & 0x8000;
    z = operand == 0x0000;
    writePackedCSR(targets::isa::packCSR<ISA>(n, z, v, c));
    break;

  // LDBx instructions depend on decodeLoadOperand to 0-fill upper byte.
  case mn::LDBA:
    temp = (a & 0xff00) | (operand & 0x00ff);
    writeReg(Register::A, temp);
    // LDBx always clears n.
    n = 0;
    z = (temp & 0xff) == 0x0000;
    writePackedCSR(targets::isa::packCSR<ISA>(n, z, v, c));
    break;
  case mn::LDBX:
    temp = (x & 0xff00) | (operand & 0x00ff);
    writeReg(Register::X, temp);
    // LDBx always clears n.
    n = 0;
    z = (temp & 0xff) == 0x0000;
    writePackedCSR(targets::isa::packCSR<ISA>(n, z, v, c));
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
  default:
    writeReg(Register::PC, pc);
    _status = Status::IllegalOpcode;
    throw std::runtime_error("Illegal Opcode");
  }

  // Increment PC and writeback
  writeReg(Register::PC, pc);
  if (_dbg) _dbg->bps->notifyPCChanged(tmp);
  return {.pause = 0, .delay = 1};
}

void targets::pep9::isa::CPU::decodeStoreOperand(quint8 is, quint16 os, quint16 &decoded, bool traced) {
  using Register = ::isa::Pep9::Register;
  using am = ::isa::Pep9::AddressingMode;
  auto acc_i = traced ? rw_i : gs_i;

  static const bool swap = bits::hostOrder() != bits::Order::BigEndian;
  auto instruction = ::isa::Pep9::opcodeLUT[is];

  switch (instruction.mode) {
  // case am::I:
  case am::D: decoded = os; break;
  case am::N:
    _memory->read(os, {reinterpret_cast<quint8 *>(&decoded), 2}, acc_i);
    if (swap) decoded = bits::byteswap(decoded);
    break;
  case am::S: decoded = os + readReg(Register::SP); break;
  case am::X: decoded = os + readReg(Register::X); break;
  case am::SX: decoded = os + readReg(Register::SP) + readReg(Register::X); break;
  case am::SF:
    _memory->read(os + readReg(Register::SP), {reinterpret_cast<quint8 *>(&decoded), 2}, acc_i);
    if (swap) decoded = bits::byteswap(decoded);
    break;
  case am::SFX:
    _memory->read(os + readReg(Register::SP), {reinterpret_cast<quint8 *>(&decoded), 2}, acc_i);
    if (swap) decoded = bits::byteswap(decoded);
    decoded += readReg(Register::X);
    break;
  default: throw std::logic_error("Invalid addressing mode");
  }
}

void targets::pep9::isa::CPU::decodeLoadOperand(quint8 is, quint16 os, quint16 &decoded, bool traced) {
  using Register = ::isa::Pep9::Register;
  using mn = ::isa::Pep9::Mnemonic;
  using am = ::isa::Pep9::AddressingMode;
  auto acc_i = traced ? rw_i : gs_i;

  static const bool swap = bits::hostOrder() != bits::Order::BigEndian;
  auto instruction = ::isa::Pep9::opcodeLUT[is];
  auto mnemon = instruction.instr.mnemon;
  bool isByte = mnemon == mn::LDBA || mnemon == mn::LDBX || mnemon == mn::CPBA || mnemon == mn::CPBX;
  quint16 mask = isByte ? 0xFF : 0xFFFF;
  switch (instruction.mode) {
  case am::I: decoded = os; break;
  case am::D:
    _memory->read(os, {reinterpret_cast<quint8 *>(&decoded) + int(isByte && swap ? 1 : 0), std::size_t(isByte ? 1 : 2)},
                  acc_i);
    if (swap) decoded = bits::byteswap(decoded);
    break;
  case am::N:
    _memory->read(os, {reinterpret_cast<quint8 *>(&decoded), 2}, acc_i);

    if (swap) decoded = bits::byteswap(decoded);
    _memory->read(decoded,
                  {reinterpret_cast<quint8 *>(&decoded) + int(isByte && swap ? 1 : 0), std::size_t(isByte ? 1 : 2)},
                  acc_i);
    if (swap) decoded = bits::byteswap(decoded);
    break;
  case am::S:
    _memory->read(os + readReg(Register::SP),
                  {reinterpret_cast<quint8 *>(&decoded) + int(isByte && swap ? 1 : 0), std::size_t(isByte ? 1 : 2)},
                  acc_i);
    if (swap) decoded = bits::byteswap(decoded);
    break;
  case am::X:
    _memory->read(os + readReg(Register::X),
                  {reinterpret_cast<quint8 *>(&decoded) + int(isByte && swap ? 1 : 0), std::size_t(isByte ? 1 : 2)},
                  acc_i);
    if (swap) decoded = bits::byteswap(decoded);
    break;
  case am::SX:
    _memory->read(os + readReg(Register::SP) + readReg(Register::X),
                  {reinterpret_cast<quint8 *>(&decoded) + int(isByte && swap ? 1 : 0), std::size_t(isByte ? 1 : 2)},
                  acc_i);
    if (swap) decoded = bits::byteswap(decoded);
    break;
  case am::SF:
    _memory->read(os + readReg(Register::SP), {reinterpret_cast<quint8 *>(&decoded), 2}, acc_i);

    if (swap) decoded = bits::byteswap(decoded);
    _memory->read(decoded,
                  {reinterpret_cast<quint8 *>(&decoded) + int(isByte && swap ? 1 : 0), std::size_t(isByte ? 1 : 2)},
                  acc_i);
    if (swap) decoded = bits::byteswap(decoded);
    break;
  case am::SFX:
    _memory->read(os + readReg(Register::SP), {reinterpret_cast<quint8 *>(&decoded), 2}, acc_i);

    if (swap) decoded = bits::byteswap(decoded);
    _memory->read(decoded + readReg(Register::X),
                  {reinterpret_cast<quint8 *>(&decoded) + int(isByte && swap ? 1 : 0), std::size_t(isByte ? 1 : 2)},
                  acc_i);
    if (swap) decoded = bits::byteswap(decoded);
    break;
  default: throw std::logic_error("Invalid addressing mode");
  }
  decoded &= mask;
}
