#include <QTest>
#include <QtCore>

#include "bits/operations/swap.hpp"
#include "sim/device/dense.hpp"
#include "targets/pep10/isa3/cpu.hpp"
#include "targets/pep10/isa3/helpers.hpp"
auto desc_mem = sim::api::device::Descriptor{
    .id = 1,
    .baseName = "ram",
    .fullName = "/ram",
};

auto desc_cpu = sim::api::device::Descriptor{
    .id = 2,
    .baseName = "cpu",
    .fullName = "/cpu",
};

auto span = sim::api::memory::Target<quint16>::AddressSpan{
    .minOffset = 0,
    .maxOffset = 0xFFFF,
};

auto make = []() {
  int i = 3;
  sim::api::device::IDGenerator gen = [&i]() { return i++; };
  auto storage =
      QSharedPointer<sim::memory::Dense<quint16>>::create(desc_mem, span);
  auto cpu = QSharedPointer<targets::pep10::isa::CPU>::create(desc_cpu, gen);
  cpu->setTarget(storage.data());
  return std::pair{storage, cpu};
};

sim::api::memory::Operation rw = {.speculative = false,
                                  .kind =
                                      sim::api::memory::Operation::Kind::data,
                                  .effectful = false};
bool taken(quint8 opcode, quint8 nzvc) {
  auto [n, z, v, c] = targets::pep10::isa::unpackCSR(nzvc);
  switch (opcode) {
  case 0x1E: // BRLE
    return n || z;
  case 0x20: // BRLT
    return n;
  case 0x22: // BREQ
    return z;
  case 0x24: // BRNE
    return !z;
  case 0x26: // BRGE
    return !n;
  case 0x28: // BRGT
    return (!n) && (!z);
  case 0x2a: // BRV
    return v;
  case 0x2c: // BRC
    return c;
  }
  return false;
}
class ISA3Pep10_BR_COND : public QObject {
  Q_OBJECT
private slots:
  void i() {
    QFETCH(quint8, opcode);
    QFETCH(quint8, nzvc);
    auto [n, z, v, c] = targets::pep10::isa::unpackCSR(nzvc);
    auto [mem, cpu] = make();
    // Loop over a subset of possible values for the target register.
    quint16 tmp;

    // Can't capture CPU directly b/c structured bindings.
    auto _cpu = cpu;
    auto rreg = [&](isa::Pep10::Register reg) -> quint16 {
      quint16 tmp = 0;
      targets::pep10::isa::readRegister(_cpu->regs(), reg, tmp, rw);
      return tmp;
    };
    auto rcsr = [&](isa::Pep10::CSR csr) {
      bool tmp = 0;
      targets::pep10::isa::readCSR(_cpu->csrs(), csr, tmp, rw);
      return tmp;
    };

    static const auto target_reg = isa::Pep10::Register::PC;
    // Intentional shorten "domain" here, because in non-microcode, all quanties
    // are 16 bits. Introduce chances for byte-swaps.
    for (uint16_t opspec = 0x00'00; static_cast<uint32_t>(opspec) + 1 < 0x02'00;
         opspec++) {

      // Object code for instruction under test.
      auto program = std::array<quint8, 3>{
          opcode, static_cast<uint8_t>((opspec >> 8) & 0xff),
          static_cast<uint8_t>(opspec & 0xff)};

      cpu->regs()->clear(0);
      cpu->csrs()->clear(0);
      targets::pep10::isa::writePackedCSR(cpu->csrs(), nzvc, rw);

      QVERIFY(mem->write(0, {program.data(), program.size()}, rw).completed);

      auto tick = cpu->tick(0);
      QCOMPARE(tick.error, sim::api::tick::Error::Success);

      QCOMPARE(rreg(isa::Pep10::Register::SP), 0);
      QCOMPARE(rreg(isa::Pep10::Register::A), 0);
      QCOMPARE(rreg(isa::Pep10::Register::X), 0);
      QCOMPARE(rreg(isa::Pep10::Register::TR), 0);
      QCOMPARE(rreg(isa::Pep10::Register::IS), opcode);
      // OS loaded the Mem[0x0001-0x0002].
      QCOMPARE(rreg(isa::Pep10::Register::OS), opspec);
      QCOMPARE(rreg(isa::Pep10::Register::PC),
               taken(opcode, nzvc) ? opspec : 3);

      // Check that target register had arithmetic performed.
      // Check that target status bits match RTL.
      QCOMPARE(rcsr(isa::Pep10::CSR::N), n);
      QCOMPARE(rcsr(isa::Pep10::CSR::Z), z);
      QCOMPARE(rcsr(isa::Pep10::CSR::V), v);
      QCOMPARE(rcsr(isa::Pep10::CSR::C), c);
    }
  }
  void i_data() {
    QTest::addColumn<quint8>("opcode");
    QTest::addColumn<quint8>("nzvc");
    auto enu = QMetaEnum::fromType<isa::Pep10::Mnemonic>();
    for (quint8 opcode : {0x1E, 0x20, 0x22, 0x24, 0x26, 0x28, 0x2a, 0x2c}) {
      auto mnemonic = isa::Pep10::opcodeLUT[opcode];
      auto mnStr = enu.valueToKey((int)mnemonic.instr.mnemon);
      for (quint8 nzvc = 0; nzvc < 0b1111; nzvc++)
        QTest::addRow("%s, nzvc=%x", mnStr, nzvc) << opcode << nzvc;
    }
  }
};

#include "brcond.test.moc"

QTEST_MAIN(ISA3Pep10_BR_COND)
