#include "cpu.hpp"
#include "bits/operations/swap.hpp"
#include <bit>

sim::api::memory::Operation rw_d = {
    .speculative = false,
    .kind = sim::api::memory::Operation::Kind::data,
    .effectful = true,
};

sim::api::memory::Operation rw_i = {
    .speculative = false,
    .kind = sim::api::memory::Operation::Kind::instruction,
    .effectful = true,
};

targets::pep10::isa::CPU::CPU(sim::api::device::Descriptor device,
                              sim::api::device::IDGenerator gen)
    : _device(device),
      _regs(
          {.id = gen(),
           .baseName = "regs",
           .fullName = _device.fullName + "/regs"},
          {.minOffset = 0, .length = quint8(::isa::Pep10::RegisterCount * 2)}),
      _csrs({.id = gen(),
             .baseName = "csrs",
             .fullName = _device.fullName + "/csrs"},
            {.minOffset = 0, .length = ::isa::Pep10::CSRCount}) {}

sim::api::memory::Target<quint8> *targets::pep10::isa::CPU::regs() {
  return &_regs;
}

sim::api::memory::Target<quint8> *targets::pep10::isa::CPU::csrs() {
  return &_csrs;
}

const sim::api::tick::Source *targets::pep10::isa::CPU::getSource() {
  return _clock;
}

void targets::pep10::isa::CPU::setSource(sim::api::tick::Source *clock) {
  _clock = clock;
}

sim::api::tick::Result
targets::pep10::isa::CPU::tick(sim::api::tick::Type currentTick) {
  using Register = ::isa::Pep10::Register;
  quint16 pc = readReg(Register::PC);
  static const auto retMemErr = [](const sim::api::memory::Result &res) {
    return sim::api::tick::Result{
        .pause = false,
        .tick_delay = 0,
        .error = res.error == sim::api::memory::Error::NeedsMMI
                     ? sim::api::tick::Error::NoMMInput
                     : sim::api::tick::Error::Terminate,
        .delay = 0};
  };
  // Instruction specifier fetch + writeback.
  quint8 is = 0;
  auto isResult = _memory->read(pc, &is, 1, rw_i);
  if (!isResult.completed)
    return retMemErr(isResult);
  writeReg(Register::IS, is);

  sim::api::tick::Result ret;
  if (::isa::Pep10::isOpcodeUnary(is)) {
    // Increment PC and writeback
    writeReg(Register::PC, pc += 1);
    // Execute unary dispatch
    ret = unaryDispatch(is);
  } else {
    // Instruction specifier fetch + writeback.
    quint16 os = 0;
    auto osResult = _memory->read(pc, reinterpret_cast<quint8 *>(&os), 2, rw_i);
    if (bits::hostOrder() != bits::Order::LittleEndian)
      os = bits::byteswap(os);
    if (!osResult.completed)
      return retMemErr(osResult);
    writeReg(Register::OS, os);
    // Execute nonunary dispatch, which is responsible for writing back PC.
    ret = nonunaryDispatch(is, os, pc += 2);
  }
  // TODO: Check for BP's
  ret.pause = false;
  return ret;
}

void targets::pep10::isa::CPU::setTraceBuffer(sim::api::trace::Buffer *tb) {
  _tb = tb;
  _regs.setTraceBuffer(tb);
  _csrs.setTraceBuffer(tb);
}

void targets::pep10::isa::CPU::trace(bool enabled) {
  if (_tb)
    _tb->trace(_device.id, enabled);
  _regs.trace(enabled);
  _csrs.trace(enabled);
}

quint8
targets::pep10::isa::CPU::packetSize(sim::api::packet::Flags flags) const {
  throw std::logic_error("unimplemented");
}

bool targets::pep10::isa::CPU::applyTrace(void *trace) {

  throw std::logic_error("unimplemented");
}

bool targets::pep10::isa::CPU::unapplyTrace(void *trace) {
  throw std::logic_error("unimplemented");
}

void targets::pep10::isa::CPU::setTarget(
    sim::api::memory::Target<quint16> *target) {
  _memory = target;
}

void targets::pep10::isa::CPU::setTarget(
    void *port, sim::api::memory::Target<quint16> *target) {
  throw std::logic_error("unimplemented");
}

quint16 targets::pep10::isa::CPU::readReg(::isa::Pep10::Register reg) {
  quint16 ret = 0;
  _regs.read(static_cast<quint8>(reg) * 2, reinterpret_cast<quint8 *>(&ret), 2,
             rw_d);
  if (bits::hostOrder() != bits::Order::LittleEndian)
    ret = bits::byteswap(ret);
  return ret;
}

void targets::pep10::isa::CPU::writeReg(::isa::Pep10::Register reg,
                                        quint16 val) {
  if (bits::hostOrder() != bits::Order::LittleEndian)
    bits::byteswap(val);
  if (!_regs
           .write(static_cast<quint8>(reg) * 2,
                  reinterpret_cast<quint8 *>(&val), 2, rw_d)
           .completed)
    throw std::logic_error("failed to write register");
}

bool targets::pep10::isa::CPU::readCSR(::isa::Pep10::CSR csr) {
  quint8 ret = 0;
  _csrs.read(static_cast<quint8>(csr), &ret, 1, rw_d);
  return ret;
}

void targets::pep10::isa::CPU::writeCSR(::isa::Pep10::CSR csr, bool val) {
  _csrs.write(static_cast<quint8>(csr), reinterpret_cast<quint8 *>(&val), 1,
              rw_d);
}

quint8 targets::pep10::isa::CPU::packCSR(bool n, bool z, bool v, bool c) {
  return (n << 3) | (z << 2) | (v << 1) | (c << 0);
}

std::tuple<bool, bool, bool, bool>
targets::pep10::isa::CPU::unpackCSR(quint8 value) {
  return {value & 0b1000, value & 0b0100, value & 0b0010, value & 0b0001};
}

quint8 targets::pep10::isa::CPU::readPackedCSR() {
  quint8 ctx[4];
  _csrs.read(0, ctx, 4, rw_d);
  return packCSR(ctx[0], ctx[1], ctx[2], ctx[3]);
}

void targets::pep10::isa::CPU::writePackedCSR(quint8 val) {
  auto [n, z, v, c] = unpackCSR(val);
  quint8 ctx[4];
  ctx[0] = n;
  ctx[1] = z;
  ctx[2] = v;
  ctx[3] = c;
  if (!_csrs.write(0, ctx, 4, rw_d).completed)
    throw std::logic_error("Failed to write CSR");
}

sim::api::tick::Result targets::pep10::isa::CPU::unaryDispatch(quint8 is) {
  using mn = ::isa::Pep10::Mnemonic;
  using Register = ::isa::Pep10::Register;

  static const bool swap = bits::hostOrder() != bits::Order::LittleEndian;
  auto mnemonic = ::isa::Pep10::opcodeLUT[is];
  quint16 a = readReg(Register::A), sp = readReg(Register::SP),
          x = readReg(Register::X);
  sim::api::memory::Result mem_res;
  quint16 tmp = 0;
  quint8 tmp8 = 0;
  // Long enough to either hold all regs or one ctx switch block.
  static const quint8 registersBytes = 2 * ::isa::Pep10::RegisterCount;
  quint8 ctx[std::max<quint64>(registersBytes, 10)];
  auto [n, z, v, c] = unpackCSR(readPackedCSR());

  auto retErr = sim::api::tick::Error::Success;
  switch (mnemonic.instr.mnemon) {
  case mn::RET:
    mem_res = _memory->read(sp, reinterpret_cast<quint8 *>(&tmp), 2, rw_d);
    // Must byteswap tmp if on big endian host, as _memory stores in little
    // endian
    if (swap)
      tmp = bits::byteswap(tmp);
    if (mem_res.error == sim::api::memory::Error::NeedsMMI) {
      retErr = sim::api::tick::Error::NoMMInput;
      break;
    } else if (!mem_res.completed) {
      tmp = 0;
      qCritical() << "Failed to access memory";
    }
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

  case mn::MOVTA:
    writeReg(Register::TR, a);
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
    writeReg(Register::A, tmp);
    writePackedCSR(packCSR(n, z, v, c));
    break;
  case mn::NEGX:
    tmp = ~x + 1;
    n = tmp & 0x8000;
    z = tmp == 0x0000;
    v = tmp == 0x8000;
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
    n = tmp * 0x8000;
    // Is zero if all bits are 0's.
    z = tmp == 0x000;
    // Carry out if register starts with low order 1.
    c = a & 0x1;
    writeReg(Register::A, tmp);
    writePackedCSR(packCSR(n, z, v, c));
    break;
  case mn::ASRX:
    // Shift all bits to the right by 1 position. Since using unsigned shift,
    // must explicitly perform sign extension by hand.
    tmp = static_cast<quint16>(x >> 1 | ((x & 0x8000) ? 1 << 15 : 0));
    // Is negative if high order bit is 1.
    n = tmp * 0x8000;
    // Is zero if all bits are 0's.
    z = tmp == 0x000;
    // Carry out if register starts with low order 1.
    c = x & 0x1;
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
    mem_res = _regs.read(0, ctx, _regs.span().length, rw_d);
    if (mem_res.error == sim::api::memory::Error::NeedsMMI) {
      retErr = sim::api::tick::Error::NoMMInput;
      break;
    } else if (!mem_res.completed)
      qCritical() << "Failed to access memory";

    // Reload NZVC
    mem_res = _memory->read(sp, reinterpret_cast<quint8 *>(&tmp8), 1, rw_d);
    if (mem_res.error == sim::api::memory::Error::NeedsMMI) {
      retErr = sim::api::tick::Error::NoMMInput;
      break;
    } else if (!mem_res.completed) {
      tmp = 0;
      qCritical() << "Failed to access memory";
    }
    writePackedCSR(tmp8);

    // Load A into ctx. No need for byteswap, _memory is little endian as are
    // regs.
    mem_res = _memory->read(sp + 1, ctx + 2 * static_cast<quint8>(Register::A),
                            2, rw_d);
    swap ? bits::byteswap(tmp) : tmp;
    if (mem_res.error == sim::api::memory::Error::NeedsMMI) {
      retErr = sim::api::tick::Error::NoMMInput;
      break;
    } else if (!mem_res.completed)
      qCritical() << "Failed to access memory";

    // Load X into ctx
    mem_res = _memory->read(sp + 3, ctx + 2 * static_cast<quint8>(Register::X),
                            2, rw_d);
    if (mem_res.error == sim::api::memory::Error::NeedsMMI) {
      retErr = sim::api::tick::Error::NoMMInput;
      break;
    } else if (!mem_res.completed)
      qCritical() << "Failed to access memory";

    // Load PC into ctx
    mem_res = _memory->read(sp + 5, ctx + 2 * static_cast<quint8>(Register::PC),
                            2, rw_d);
    if (mem_res.error == sim::api::memory::Error::NeedsMMI) {
      retErr = sim::api::tick::Error::NoMMInput;
      break;
    } else if (!mem_res.completed)
      qCritical() << "Failed to access memory";

    // Load SP into ctx
    mem_res = _memory->read(sp + 7, ctx + 2 * static_cast<quint8>(Register::SP),
                            2, rw_d);
    if (mem_res.error == sim::api::memory::Error::NeedsMMI) {
      retErr = sim::api::tick::Error::NoMMInput;
      break;
    } else if (!mem_res.completed)
      qCritical() << "Failed to access memory";

    // Bulk write-back regs, saving a number of bits on trace metadata.
    if (!_regs.write(0, ctx, registersBytes, rw_d).completed)
      throw std::logic_error("Failed to write registers");

    tmp = sp + 10;
    // Using "host"'s variables, so byte swap if necessary.
    if (swap)
      tmp = bits::byteswap(tmp);
    mem_res = _memory->write(
        static_cast<quint16>(::isa::Pep10::MemoryVectors::SystemStackPtr),
        reinterpret_cast<quint8 *>(&tmp), 2, rw_d);
    if (!mem_res.completed)
      qCritical() << "Failed to access memory";
    break;

  case mn::USCALL:
    [[fallthrough]];
  case mn::SCALL:
    // Must byteswap because we are using "host" variables.
    ctx[0] = readPackedCSR();
    tmp = swap ? bits::byteswap(a) : a;
    bits::memcpy(ctx + 1, &tmp, 2);
    tmp = swap ? bits::byteswap(x) : x;
    bits::memcpy(ctx + 3, &tmp, 2);
    tmp = readReg(Register::PC);
    tmp = swap ? bits::byteswap(tmp) : tmp;
    bits::memcpy(ctx + 5, &tmp, 2);
    tmp = swap ? bits::byteswap(sp) : sp;
    bits::memcpy(ctx + 7, &tmp, 2);
    ctx[9] = is;

    // Read system stack address.
    mem_res = _memory->read(
        static_cast<quint16>(::isa::Pep10::MemoryVectors::SystemStackPtr),
        reinterpret_cast<quint8 *>(&tmp), 2, rw_d);
    if (mem_res.error == sim::api::memory::Error::NeedsMMI) {
      retErr = sim::api::tick::Error::NoMMInput;
      break;
    } else if (!mem_res.completed) {
      tmp = 0;
      qCritical() << "Failed to access memory";
    }
    if (swap)
      tmp = bits::byteswap(tmp);

    // Allocate ctx frame with -=.
    mem_res = _memory->write(tmp -= 10, ctx, 10, rw_d);
    if (!mem_res.completed)
      qCritical() << "Failed to access memory";
    // And update SP with OS's SP.
    writeReg(Register::SP, tmp);

    // Read trap handler pc.
    mem_res = _memory->read(
        static_cast<quint16>(::isa::Pep10::MemoryVectors::TrapHandler),
        reinterpret_cast<quint8 *>(&tmp), 2, rw_d);
    if (mem_res.error == sim::api::memory::Error::NeedsMMI)
      retErr = sim::api::tick::Error::NoMMInput;
    else if (!mem_res.completed)
      qCritical() << "Failed to access memory";
    if (swap)
      tmp = bits::byteswap(tmp);
    writeReg(Register::PC, tmp);
  default:
    retErr = sim::api::tick::Error::Terminate;
  }
  return {.pause = false,
          .tick_delay = 0,
          .error = retErr,
          .delay = sim::api::tick::Type(
              retErr != sim::api::tick::Error::Success ? 1 : 0)};
}

sim::api::tick::Result
targets::pep10::isa::CPU::nonunaryDispatch(quint8 is, quint16 os, quint16 pc) {

  using mn = ::isa::Pep10::Mnemonic;
  using Register = ::isa::Pep10::Register;

  static const bool swap = bits::hostOrder() != bits::Order::LittleEndian;
  auto mnemonic = ::isa::Pep10::opcodeLUT[is];
  quint16 a = readReg(Register::A), sp = readReg(Register::SP),
          x = readReg(Register::X);
  sim::api::memory::Result mem_res;

  quint16 operand = 0;

  quint16 tmp = 0;
  auto [n, z, v, c] = unpackCSR(readPackedCSR());

  auto instrDef = ::isa::Pep10::opcodeLUT[is];
  auto retErr = sim::api::tick::Error::Success;
  if (::isa::Pep10::isValidAddressingMode(instrDef.instr.mnemon,
                                          instrDef.mode)) {
    mem_res = ::isa::Pep10::isStore(is) ? decodeStoreOperand(is, os, operand)
                                        : decodeLoadOperand(is, os, operand);
    if (mem_res.error == sim::api::memory::Error::NeedsMMI)
      retErr = sim::api::tick::Error::NoMMInput;
    else if (!mem_res.completed)
      qCritical() << "Failed to access memory";
  } else
    retErr = sim::api::tick::Error::Terminate;

  if (retErr != sim::api::tick::Error::Success)
    return {
        .tick_delay = 0,
        .error = retErr,
        .delay = 0,
    };

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
    mem_res =
        _memory->write(sp -= 2, reinterpret_cast<quint8 *>(&tmp), 2, rw_d);
    if (mem_res.error == sim::api::memory::Error::NeedsMMI) {
      retErr = sim::api::tick::Error::NoMMInput;
      break;
    } else if (!mem_res.completed)
      qCritical() << "Failed to access memory";
    pc = operand;
    writeReg(Register::SP, sp);
    break;

  case mn::LDWT:
    writeReg(Register::TR, operand);
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
    mem_res =
        _memory->write(operand, reinterpret_cast<quint8 *>(&tmp), 2, rw_d);
    if (!mem_res.completed)
      qCritical() << "Failed to access memory";
    break;
  case mn::STWX:
    tmp = swap ? bits::byteswap(x) : x;
    mem_res =
        _memory->write(operand, reinterpret_cast<quint8 *>(&tmp), 2, rw_d);
    if (!mem_res.completed)
      qCritical() << "Failed to access memory";
    break;

  case mn::STBA:
    tmp = swap ? bits::byteswap(a) : a;
    mem_res =
        _memory->write(operand, reinterpret_cast<quint8 *>(&tmp), 1, rw_d);
    if (!mem_res.completed)
      qCritical() << "Failed to access memory";
    break;
  case mn::STBX:
    tmp = swap ? bits::byteswap(x) : x;
    mem_res =
        _memory->write(operand, reinterpret_cast<quint8 *>(&tmp), 1, rw_d);
    if (!mem_res.completed)
      qCritical() << "Failed to access memory";
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
    v = (~(a ^ operand) & (a ^ tmp)) >> 15;
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
    v = (~(x ^ operand) & (x ^ tmp)) >> 15;
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
    v = (~(a ^ operand) & (a ^ tmp)) >> 15;
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
    v = (~(x ^ operand) & (x ^ tmp)) >> 15;
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
    retErr = sim::api::tick::Error::Terminate;
  }

  // Increment PC and writeback
  writeReg(Register::PC, pc);
  return {.pause = false,
          .tick_delay = 0,
          .error = retErr,
          .delay = sim::api::tick::Type(
              (retErr == sim::api::tick::Error::Success) ? 1 : 0)};
}

sim::api::memory::Result
targets::pep10::isa::CPU::decodeStoreOperand(quint8 is, quint16 os,
                                             quint16 &decoded) {
  using Register = ::isa::Pep10::Register;
  using am = ::isa::Pep10::AddressingMode;

  static const bool swap = bits::hostOrder() != bits::Order::LittleEndian;
  auto instruction = ::isa::Pep10::opcodeLUT[is];
  sim::api::memory::Result mem_res = {
      .completed = true,
      .pause = false,
      .error = sim::api::memory::Error::Success,
  };

  switch (instruction.mode) {
  // case am::I:
  case am::D:
    decoded = os;
    break;
  case am::N:
    mem_res = _memory->read(os, reinterpret_cast<quint8 *>(&decoded), 2, rw_i);
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
    mem_res = _memory->read(os + readReg(Register::SP),
                            reinterpret_cast<quint8 *>(&decoded), 2, rw_i);
    if (swap)
      decoded = bits::byteswap(decoded);
    break;
  case am::SFX:
    mem_res = _memory->read(os + readReg(Register::SP),
                            reinterpret_cast<quint8 *>(&decoded), 2, rw_i);
    decoded += readReg(Register::X);
    if (swap)
      decoded = bits::byteswap(decoded);
    break;
  }
  return mem_res;
}

sim::api::memory::Result
targets::pep10::isa::CPU::decodeLoadOperand(quint8 is, quint16 os,
                                            quint16 &decoded) {
  using Register = ::isa::Pep10::Register;
  using mn = ::isa::Pep10::Mnemonic;
  using am = ::isa::Pep10::AddressingMode;

  static const bool swap = bits::hostOrder() != bits::Order::LittleEndian;
  auto instruction = ::isa::Pep10::opcodeLUT[is];
  sim::api::memory::Result mem_res = {
      .completed = true,
      .pause = false,
      .error = sim::api::memory::Error::Success,
  };
  auto mnemon = instruction.instr.mnemon;
  bool isByte = mnemon == mn::LDBA || mnemon == mn::LDBX ||
                mnemon == mn::CPBA || mnemon == mn::CPBX;
  quint16 mask = isByte ? 0xFF : 0xFFFF;
  switch (instruction.mode) {
  case am::I:
    decoded = os;
    break;
  case am::D:
    mem_res = _memory->read(os, reinterpret_cast<quint8 *>(&decoded),
                            isByte ? 1 : 2, rw_i);
    if (swap)
      decoded = bits::byteswap(decoded);
    break;
  case am::N:
    mem_res = _memory->read(os, reinterpret_cast<quint8 *>(&decoded), 2, rw_i);
    if (!mem_res.completed)
      return mem_res;

    if (swap)
      decoded = bits::byteswap(decoded);
    mem_res = _memory->read(decoded, reinterpret_cast<quint8 *>(&decoded),
                            isByte ? 1 : 2, rw_i);
    if (swap)
      decoded = bits::byteswap(decoded);
    break;
  case am::S:
    mem_res = _memory->read(os + readReg(Register::SP),
                            reinterpret_cast<quint8 *>(&decoded),
                            isByte ? 1 : 2, rw_i);
    if (swap)
      decoded = bits::byteswap(decoded);
    break;
  case am::X:
    mem_res = _memory->read(os + readReg(Register::X),
                            reinterpret_cast<quint8 *>(&decoded),
                            isByte ? 1 : 2, rw_i);
    if (swap)
      decoded = bits::byteswap(decoded);
    break;
  case am::SX:
    mem_res = _memory->read(os + readReg(Register::SP) + readReg(Register::X),
                            reinterpret_cast<quint8 *>(&decoded),
                            isByte ? 1 : 2, rw_i);
    if (swap)
      decoded = bits::byteswap(decoded);
    break;
  case am::SF:
    mem_res = _memory->read(os + readReg(Register::SP),
                            reinterpret_cast<quint8 *>(&decoded), 2, rw_i);
    if (!mem_res.completed)
      return mem_res;

    if (swap)
      decoded = bits::byteswap(decoded);
    mem_res = _memory->read(decoded, reinterpret_cast<quint8 *>(&decoded),
                            isByte ? 1 : 2, rw_i);
    if (swap)
      decoded = bits::byteswap(decoded);
    break;
  case am::SFX:
    mem_res = _memory->read(os + readReg(Register::SP),
                            reinterpret_cast<quint8 *>(&decoded), 2, rw_i);
    if (!mem_res.completed)
      return mem_res;

    if (swap)
      decoded = bits::byteswap(decoded);
    mem_res = _memory->read(decoded + readReg(Register::X),
                            reinterpret_cast<quint8 *>(&decoded),
                            isByte ? 1 : 2, rw_i);
    if (swap)
      decoded = bits::byteswap(decoded);
    break;
  }
  decoded &= mask;
  return mem_res;
}
