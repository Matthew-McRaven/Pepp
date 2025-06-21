#include "targets/pep9/mc2/cpu.hpp"
#include "toolchain/ucode/uarch.hpp"
#include "utils/bits/span.hpp"
#include "utils/bits/swap.hpp"

namespace {
sim::api2::memory::Operation gs_d = {
    .type = sim::api2::memory::Operation::Type::Application,
    .kind = sim::api2::memory::Operation::Kind::data,
};
sim::api2::memory::Operation rw_d = {
    .type = sim::api2::memory::Operation::Type::Standard,
    .kind = sim::api2::memory::Operation::Kind::data,
};
} // namespace
targets::pep9::mc2::CPUByteBus::CPUByteBus(sim::api2::device::Descriptor device, sim::api2::device::IDGenerator gen)
    : _device(device),
      _bankRegs({.id = gen(), .baseName = "regs", .fullName = _device.fullName + "/regs"},
                sim::api2::memory::AddressSpan<quint8>(0, quint8(::pepp::ucode::Pep9Registers::register_count() - 1))),
      _hiddenRegs({.id = gen(), .baseName = "hidden", .fullName = _device.fullName + "/hidden"},
                  sim::api2::memory::AddressSpan<quint8>(0, ::pepp::ucode::Pep9ByteBus::hidden_register_count() - 1)),
      _csrs({.id = gen(), .baseName = "csrs", .fullName = _device.fullName + "/csrs"},
            sim::api2::memory::AddressSpan<quint8>(0, quint8(::pepp::ucode::Pep9Registers::csr_count() - 1))) {}

sim::api2::memory::Target<quint8> *targets::pep9::mc2::CPUByteBus::bankRegs() { return &_bankRegs; }

sim::api2::memory::Target<quint8> *targets::pep9::mc2::CPUByteBus::hiddenRegs() { return &_hiddenRegs; }

sim::api2::memory::Target<quint8> *targets::pep9::mc2::CPUByteBus::csrs() { return &_csrs; }

sim::api2::device::Descriptor targets::pep9::mc2::CPUByteBus::device() const { return _device; }

void targets::pep9::mc2::CPUByteBus::setMicrocode(std::vector<pepp::ucode::Pep9ByteBus::Code> &&code) {
  _microcode = std::move(code);
}

const std::span<const pepp::ucode::Pep9ByteBus::Code> targets::pep9::mc2::CPUByteBus::microcode() { return _microcode; }

void targets::pep9::mc2::CPUByteBus::applyPreconditions(
    const QList<pepp::ucode::Test<pepp::ucode::Pep9Registers>> &tests) {
  for (const auto &test : tests) {
    if (std::holds_alternative<pepp::ucode::MemTest>(test)) {
      auto memTest = std::get<pepp::ucode::MemTest>(test);
      _memory->write(memTest.address, {reinterpret_cast<const quint8 *>(&memTest.value), 2}, gs_d);
    } else if (std::holds_alternative<pepp::ucode::RegisterTest<pepp::ucode::Pep9Registers>>(test)) {
      auto regTest = std::get<pepp::ucode::RegisterTest<pepp::ucode::Pep9Registers>>(test);
      const quint8 size = pepp::ucode::Pep9Registers::register_byte_size(regTest.reg);
      quint32 regValue = regTest.value;
      if (bits::hostOrder() != bits::Order::BigEndian) {
        regValue = bits::byteswap(regTest.value);
        regValue >>= (8 * (4 - size)); // Must move bytes around since we are only using part of regValue.
      };
      _bankRegs.write(static_cast<quint8>(regTest.reg), {reinterpret_cast<quint8 *>(&regValue), size}, gs_d);
    }
  }
}

std::vector<bool>
targets::pep9::mc2::CPUByteBus::testPostconditions(const QList<pepp::ucode::Test<pepp::ucode::Pep9Registers>> &tests) {
  return {};
}

void targets::pep9::mc2::CPUByteBus::setConstantRegisters() {
  writeReg(22, 0x00);
  writeReg(23, 0x01);
  writeReg(24, 0x02);
  writeReg(25, 0x03);
  writeReg(26, 0x04);
  writeReg(27, 0x08);
  writeReg(28, 0xF0);
  writeReg(29, 0xF6);
  writeReg(30, 0xFE);
  writeReg(31, 0xFF);
}

targets::pep9::mc2::CPUByteBus::Status targets::pep9::mc2::CPUByteBus::status() const { return _status; }

void targets::pep9::mc2::CPUByteBus::resetMicroPC() { _microPC = 0; }

const sim::api2::tick::Source *targets::pep9::mc2::CPUByteBus::getSource() { return _clock; }

void targets::pep9::mc2::CPUByteBus::setSource(sim::api2::tick::Source *clock) { _clock = clock; }

struct alu_result {
  bool n = false, z = false, v = false, c = false;
  quint8 value = 0;
};

sim::api2::tick::Result targets::pep9::mc2::CPUByteBus::clock(sim::api2::tick::Type currentTick) {
  sim::api2::tick::Result ret;
  const auto &code = _microcode[_microPC++];
  quint8 A = readReg(code.A), B = readReg(code.B), MDR = readHidden(HiddenRegisters::MDR), c_out = 0, a_in = 0;
  bool c_in = code.CSMux == 0 ? readCSR(CSRs::C) : readCSR(CSRs::S);
  quint16 MAR = (readHidden(HiddenRegisters::MARA) << 8) | readHidden(HiddenRegisters::MARB);
  quint8 memtemp[1] = {0};

  a_in = code.AMux == 0 ? MDR : A;
  alu_result alu;
  alu.value = pepp::ucode::detail::pep9_1byte::computeALU(code.ALU, a_in, B, c_in, alu.n, alu.z, alu.v, alu.c);

  if (code.MemWrite) {
    memtemp[0] = MDR;
    // Done pre-increment to avoid dealing with wrapping.
    if (memStatus.onCycle == 2) _memory->write(MAR, {reinterpret_cast<quint8 *>(&memtemp), 1}, rw_d);
    memStatus.onCycle = (memStatus.onCycle % 3) + 1; // increments and wraps cycle 3 to 1.
  } else if (code.MemRead) {
    if (memStatus.onCycle == 2) _memory->read(MAR, {reinterpret_cast<quint8 *>(&memtemp), 1}, rw_d);
    memStatus.onCycle = (memStatus.onCycle % 3) + 1; // increments and wraps cycle 3 to 1.
  } else memStatus.onCycle = 0;

  if (code.MARCk) {
    if (memStatus.onCycle != 0) _status = Status::ChangedAddress;
    else writeHidden(HiddenRegisters::MARA, A), writeHidden(HiddenRegisters::MARB, B);
  }
  if (code.NCk) writeCSR(CSRs::N, alu.n);
  if (code.ZCk) writeCSR(CSRs::Z, alu.z && ((code.AndZ == 1 && readCSR(CSRs::Z)) || code.AndZ == 0));
  if (code.VCk) writeCSR(CSRs::V, alu.v);
  if (code.CCk) writeCSR(CSRs::C, alu.c);
  if (code.SCk) writeCSR(CSRs::S, alu.c);
  if (code.CMux == 0) {
    c_out = readCSR(CSRs::N) ? 0x8 : 0;
    c_out |= readCSR(CSRs::Z) ? 0x4 : 0;
    c_out |= readCSR(CSRs::V) ? 0x2 : 0;
    c_out |= readCSR(CSRs::C) ? 0x1 : 0;
  } else c_out = alu.value;
  if (code.MDRCk && code.MDRMux == 1) {
    // TODO: validate that I chose the right cycle number
    if (memStatus.onCycle > 1) _status = Status::ChangedData;
    writeHidden(HiddenRegisters::MDR, c_out);
  } else if (code.MDRCk) {
    if (memStatus.onCycle == 3) writeHidden(HiddenRegisters::MDR, memtemp[0]);
    else _status = Status::MemoryTooSoon;
  }
  // Prevent writing to "read only" registers
  if (code.CCk && code.C < 22) writeReg(code.C, c_out);

  if (_microPC == _microcode.size()) _status = Status::Halted;
  ret.pause = _status != Status::Ok;
  return ret;
}

bool targets::pep9::mc2::CPUByteBus::analyze(sim::api2::trace::PacketIterator iter, sim::api2::trace::Direction) {
  // TODO: update micropc
  return false;
}

void targets::pep9::mc2::CPUByteBus::trace(bool enabled) {
  if (_tb) _tb->trace(_device.id, enabled);
  _bankRegs.trace(enabled);
  _hiddenRegs.trace(enabled);
  _csrs.trace(enabled);
}

void targets::pep9::mc2::CPUByteBus::setBuffer(sim::api2::trace::Buffer *tb) {
  _tb = tb;
  _bankRegs.setBuffer(tb);
  _hiddenRegs.setBuffer(tb);
  _csrs.setBuffer(tb);
}

void targets::pep9::mc2::CPUByteBus::setTarget(sim::api2::memory::Target<quint16> *target, void *port) {
  _memory = target;
}

void targets::pep9::mc2::CPUByteBus::setDebugger(pepp::debug::Debugger *debugger) { _dbg = debugger; }

void targets::pep9::mc2::CPUByteBus::clearDebugger() { _dbg = nullptr; }

quint8 targets::pep9::mc2::CPUByteBus::readReg(quint8 reg) {
  quint8 ret = 0;
  (void)_bankRegs.read(reg, {&ret, 1}, rw_d);
  return ret;
}

void targets::pep9::mc2::CPUByteBus::writeReg(quint8 reg, quint8 val) { (void)_bankRegs.write(reg, {&val, 1}, rw_d); }

quint8 targets::pep9::mc2::CPUByteBus::readHidden(HiddenRegisters reg) {
  quint8 ret = 0;
  (void)_hiddenRegs.read(static_cast<quint8>(reg), {&ret, 1}, rw_d);
  return ret;
}

void targets::pep9::mc2::CPUByteBus::writeHidden(HiddenRegisters reg, quint8 val) {
  (void)_hiddenRegs.write(static_cast<quint8>(reg), {&val, 1}, rw_d);
}

bool targets::pep9::mc2::CPUByteBus::readCSR(CSRs reg) {
  quint8 ret = 0;
  (void)_csrs.read(static_cast<quint8>(reg), {&ret, 1}, rw_d);
  return ret;
}

void targets::pep9::mc2::CPUByteBus::writeCSR(CSRs reg, bool val) {
  quint8 tmp = val ? 1 : 0;
  (void)_csrs.write(static_cast<quint8>(reg), {&tmp, 1}, rw_d);
}

targets::pep9::mc2::CPUWordBus::CPUWordBus(sim::api2::device::Descriptor device, sim::api2::device::IDGenerator gen)
    : _device(device),
      _bankRegs({.id = gen(), .baseName = "regs", .fullName = _device.fullName + "/regs"},
                sim::api2::memory::AddressSpan<quint8>(0, quint8(::pepp::ucode::Pep9Registers::register_count() - 1))),
      _hiddenRegs({.id = gen(), .baseName = "hidden", .fullName = _device.fullName + "/hidden"},
                  sim::api2::memory::AddressSpan<quint8>(0, ::pepp::ucode::Pep9WordBus::hidden_register_count() - 1)),
      _csrs({.id = gen(), .baseName = "csrs", .fullName = _device.fullName + "/csrs"},
            sim::api2::memory::AddressSpan<quint8>(0, quint8(::pepp::ucode::Pep9Registers::csr_count() - 1))) {}

sim::api2::memory::Target<quint8> *targets::pep9::mc2::CPUWordBus::bankRegs() { return &_bankRegs; }

sim::api2::memory::Target<quint8> *targets::pep9::mc2::CPUWordBus::hiddenRegs() { return &_hiddenRegs; }

sim::api2::memory::Target<quint8> *targets::pep9::mc2::CPUWordBus::csrs() { return &_csrs; }

sim::api2::device::Descriptor targets::pep9::mc2::CPUWordBus::device() const { return _device; }

void targets::pep9::mc2::CPUWordBus::setMicrocode(std::vector<pepp::ucode::Pep9WordBus::Code> &&code) {
  _microcode = std::move(code);
}

const std::span<const pepp::ucode::Pep9WordBus::Code> targets::pep9::mc2::CPUWordBus::microcode() { return _microcode; }

void targets::pep9::mc2::CPUWordBus::applyPreconditions(
    const QList<pepp::ucode::Test<pepp::ucode::Pep9Registers>> &tests) {
  for (const auto &test : tests) {
    if (std::holds_alternative<pepp::ucode::MemTest>(test)) {
      auto memTest = std::get<pepp::ucode::MemTest>(test);
      _memory->write(memTest.address, {reinterpret_cast<const quint8 *>(&memTest.value), 2}, gs_d);
    } else if (std::holds_alternative<pepp::ucode::RegisterTest<pepp::ucode::Pep9Registers>>(test)) {
      auto regTest = std::get<pepp::ucode::RegisterTest<pepp::ucode::Pep9Registers>>(test);
      const quint8 size = pepp::ucode::Pep9Registers::register_byte_size(regTest.reg);
      quint32 regValue = regTest.value;
      if (bits::hostOrder() != bits::Order::BigEndian) {
        regValue = bits::byteswap(regTest.value);
        regValue >>= (8 * (4 - size)); // Must move bytes around since we are only using part of regValue.
      };
      _bankRegs.write(static_cast<quint8>(regTest.reg), {reinterpret_cast<quint8 *>(&regValue), size}, gs_d);
    }
  }
}

std::vector<bool>
targets::pep9::mc2::CPUWordBus::testPostconditions(const QList<pepp::ucode::Test<pepp::ucode::Pep9Registers>> &tests) {
  return {};
}

void targets::pep9::mc2::CPUWordBus::setConstantRegisters() {
  writeReg(22, 0x00);
  writeReg(23, 0x01);
  writeReg(24, 0x02);
  writeReg(25, 0x03);
  writeReg(26, 0x04);
  writeReg(27, 0x08);
  writeReg(28, 0xF0);
  writeReg(29, 0xF6);
  writeReg(30, 0xFE);
  writeReg(31, 0xFF);
}

targets::pep9::mc2::CPUWordBus::Status targets::pep9::mc2::CPUWordBus::status() const { return _status; }

void targets::pep9::mc2::CPUWordBus::resetMicroPC() { _microPC = 0; }

const sim::api2::tick::Source *targets::pep9::mc2::CPUWordBus::getSource() { return _clock; }

void targets::pep9::mc2::CPUWordBus::setSource(sim::api2::tick::Source *clock) { _clock = clock; }

sim::api2::tick::Result targets::pep9::mc2::CPUWordBus::clock(sim::api2::tick::Type currentTick) {
  sim::api2::tick::Result ret;
  const auto &code = _microcode[_microPC++];
  quint8 A = readReg(code.A), B = readReg(code.B), MDRE = readHidden(HiddenRegisters::MDRE),
         MDRO = readHidden(HiddenRegisters::MDRO), c_out = 0, a_in = 0;
  bool c_in = code.CSMux == 0 ? readCSR(CSRs::C) : readCSR(CSRs::S);
  quint16 MAR =
      ((readHidden(HiddenRegisters::MARA) << 8) | readHidden(HiddenRegisters::MARB)) & ~0x01; // Mask to even address
  quint8 memtemp[2] = {0, 0};

  if (code.AMux == 0) {
    if (code.EOMux == 0) a_in = MDRE;
    else a_in = MDRO;
  } else a_in = A;
  alu_result alu;
  alu.value = pepp::ucode::detail::pep9_1byte::computeALU(code.ALU, a_in, B, c_in, alu.n, alu.z, alu.v, alu.c);

  if (code.MemWrite) {
    memtemp[0] = MDRE, memtemp[1] = MDRO;
    // Done pre-increment to avoid dealing with wrapping.
    if (memStatus.onCycle == 2) _memory->write(MAR, {reinterpret_cast<quint8 *>(&memtemp), 2}, rw_d);
    memStatus.onCycle = (memStatus.onCycle % 3) + 1; // increments and wraps cycle 3 to 1.
  } else if (code.MemRead) {
    if (memStatus.onCycle == 2) _memory->read(MAR, {reinterpret_cast<quint8 *>(&memtemp), 2}, rw_d);
    memStatus.onCycle = (memStatus.onCycle % 3) + 1; // increments and wraps cycle 3 to 1.
  } else memStatus.onCycle = 0;

  if (code.MARCk) {
    if (memStatus.onCycle != 0) _status = Status::ChangedAddress;
    else if (code.MARMux == 0) writeHidden(HiddenRegisters::MARA, MDRE), writeHidden(HiddenRegisters::MARB, MDRO);
    else writeHidden(HiddenRegisters::MARA, A), writeHidden(HiddenRegisters::MARB, B);
  }
  if (code.NCk) writeCSR(CSRs::N, alu.n);
  if (code.ZCk) writeCSR(CSRs::Z, alu.z && ((code.AndZ == 1 && readCSR(CSRs::Z)) || code.AndZ == 0));
  if (code.VCk) writeCSR(CSRs::V, alu.v);
  if (code.CCk) writeCSR(CSRs::C, alu.c);
  if (code.SCk) writeCSR(CSRs::S, alu.c);
  if (code.CMux == 0) {
    c_out = readCSR(CSRs::N) ? 0x8 : 0;
    c_out |= readCSR(CSRs::Z) ? 0x4 : 0;
    c_out |= readCSR(CSRs::V) ? 0x2 : 0;
    c_out |= readCSR(CSRs::C) ? 0x1 : 0;
  } else c_out = alu.value;

  if (code.MDRECk && code.MDREMux == 1) {
    // TODO: validate that I chose the right cycle number
    if (memStatus.onCycle > 1) _status = Status::ChangedData;
    writeHidden(HiddenRegisters::MDRE, c_out);
  } else if (code.MDRECk) {
    if (memStatus.onCycle == 3) writeHidden(HiddenRegisters::MDRE, memtemp[0]);
    else _status = Status::MemoryTooSoon;
  }
  if (code.MDROCk && code.MDROMux == 1) {
    // TODO: validate that I chose the right cycle number
    if (memStatus.onCycle > 1) _status = Status::ChangedData;
    writeHidden(HiddenRegisters::MDRO, c_out);
  } else if (code.MDROCk) {
    if (memStatus.onCycle == 3) writeHidden(HiddenRegisters::MDRO, memtemp[1]);
    else _status = Status::MemoryTooSoon;
  }

  // Prevent writing to "read only" registers
  if (code.CCk && code.C < 22) writeReg(code.C, c_out);

  if (_microPC == _microcode.size()) _status = Status::Halted;
  ret.pause = _status != Status::Ok;
  return ret;
}

bool targets::pep9::mc2::CPUWordBus::analyze(sim::api2::trace::PacketIterator iter, sim::api2::trace::Direction) {
  // TODO: update micropc
  return false;
}

void targets::pep9::mc2::CPUWordBus::trace(bool enabled) {
  if (_tb) _tb->trace(_device.id, enabled);
  _bankRegs.trace(enabled);
  _hiddenRegs.trace(enabled);
  _csrs.trace(enabled);
}

void targets::pep9::mc2::CPUWordBus::setBuffer(sim::api2::trace::Buffer *tb) {
  _tb = tb;
  _bankRegs.setBuffer(tb);
  _hiddenRegs.setBuffer(tb);
  _csrs.setBuffer(tb);
}

void targets::pep9::mc2::CPUWordBus::setTarget(sim::api2::memory::Target<quint16> *target, void *port) {
  _memory = target;
}

void targets::pep9::mc2::CPUWordBus::setDebugger(pepp::debug::Debugger *debugger) { _dbg = debugger; }

void targets::pep9::mc2::CPUWordBus::clearDebugger() { _dbg = nullptr; }

quint8 targets::pep9::mc2::CPUWordBus::readReg(quint8 reg) {
  quint8 ret = 0;
  (void)_bankRegs.read(reg, {&ret, 1}, rw_d);
  return ret;
}

void targets::pep9::mc2::CPUWordBus::writeReg(quint8 reg, quint8 val) { (void)_bankRegs.write(reg, {&val, 1}, rw_d); }

quint8 targets::pep9::mc2::CPUWordBus::readHidden(HiddenRegisters reg) {
  quint8 ret = 0;
  (void)_hiddenRegs.read(static_cast<quint8>(reg), {&ret, 1}, rw_d);
  return ret;
}

void targets::pep9::mc2::CPUWordBus::writeHidden(HiddenRegisters reg, quint8 val) {
  (void)_hiddenRegs.write(static_cast<quint8>(reg), {&val, 1}, rw_d);
}

bool targets::pep9::mc2::CPUWordBus::readCSR(CSRs reg) {
  quint8 ret = 0;
  (void)_csrs.read(static_cast<quint8>(reg), {&ret, 1}, rw_d);
  return ret;
}

void targets::pep9::mc2::CPUWordBus::writeCSR(CSRs reg, bool val) {
  quint8 tmp = val ? 1 : 0;
  (void)_csrs.write(static_cast<quint8>(reg), {&tmp, 1}, rw_d);
}
