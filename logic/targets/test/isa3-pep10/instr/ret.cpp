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

class ISA3Pep10_RET : public QObject {
  Q_OBJECT
private slots:
  void u() {
    auto [mem, cpu] = make();

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

    // Object code for instruction under test.
    auto program = std::array<quint8, 1>{(quint8)isa::Pep10::Mnemonic::RET};

    cpu->regs()->clear(0);
    cpu->csrs()->clear(0);
    auto spAddr = 0x27;
    quint16 retAddr = 0xBEEF;
    quint16 tmp = bits::hostOrder() != bits::Order::BigEndian
                      ? bits::byteswap(retAddr)
                      : retAddr;
    mem->write(
        spAddr,
        bits::span<quint8>{reinterpret_cast<quint8 *>(&tmp), sizeof(tmp)}, rw);
    targets::pep10::isa::writeRegister(cpu->regs(), isa::Pep10::Register::SP,
                                       spAddr, rw);

    QVERIFY(mem->write(0, {program.data(), program.size()}, rw).completed);

    auto tick = cpu->tick(0);
    QCOMPARE(tick.error, sim::api::tick::Error::Success);

    QCOMPARE(rreg(isa::Pep10::Register::SP), spAddr + 2);
    QCOMPARE(rreg(isa::Pep10::Register::A), 0);
    QCOMPARE(rreg(isa::Pep10::Register::X), 0);
    QCOMPARE(rreg(isa::Pep10::Register::PC), retAddr);
    QCOMPARE(rreg(isa::Pep10::Register::IS), (quint8)isa::Pep10::Mnemonic::RET);
    QCOMPARE(rreg(isa::Pep10::Register::OS), 0);
  }
};

#include "ret.moc"

QTEST_MAIN(ISA3Pep10_RET)