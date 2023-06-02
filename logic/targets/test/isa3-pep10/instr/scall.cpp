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

class ISA3Pep10_SCALL : public QObject {
  Q_OBJECT
private slots:
  void i() {
    auto [mem, cpu] = make();

    // Can't capture CPU directly b/c structured bindings.
    auto _cpu = cpu;
    auto rreg = [&](isa::Pep10::Register reg) -> quint16 {
      quint16 tmp = 0;
      targets::pep10::isa::readRegister(_cpu->regs(), reg, tmp, rw);
      return tmp;
    };

    quint16 tmp=0;
    auto tmpSpan = bits::span<quint8>{reinterpret_cast<quint8*>(tmp), sizeof(tmp)};
    //const quint8 truth[] = {(quint8) isa::Pep10::Mnemonic::SCALL, /*sp*/ 0xFE, 0xED, /*PC*/ 0x00,0x01, /*X*/ 0xBA, 0xAD, /*A*/ 0x11,0x22, /*NZVC*/ 0b1101};
    const quint8 truth[] = {/*NZVC*/ 0b1101, /*A*/ 0x11,0x22, /*X*/ 0xBA, 0xAD, /*PC*/ 0x00,0x01, /*sp*/ 0xFE, 0xED,(quint8) isa::Pep10::Mnemonic::SCALL};
    quint8 buf[sizeof(truth)];
    auto program = std::array<quint8, 3>{(quint8) isa::Pep10::Mnemonic::SCALL, 0xCA, 0xBE};
    auto osSP = std::array<quint8, 2>{0x80,0x86};

    cpu->regs()->clear(0);
    cpu->csrs()->clear(0);
    QVERIFY(targets::pep10::isa::writePackedCSR(cpu->csrs(), truth[0], rw).completed);
    tmp = bits::hostOrder() != bits::Order::BigEndian ? bits::byteswap(*(quint16*)(truth + 1))
                                                      : *(quint16*)(truth + 1);
    QVERIFY(targets::pep10::isa::writeRegister(cpu->regs(), isa::Pep10::Register::A, tmp, rw).completed);
    tmp = bits::hostOrder() != bits::Order::BigEndian ? bits::byteswap(*(quint16*)(truth + 3))
                                                      : *(quint16*)(truth + 3);
    QVERIFY(targets::pep10::isa::writeRegister(cpu->regs(), isa::Pep10::Register::X, tmp, rw).completed);
    tmp = bits::hostOrder() != bits::Order::BigEndian ? bits::byteswap(*(quint16*)(truth + 7))
                                                      : *(quint16*)(truth + 7);
    QVERIFY(targets::pep10::isa::writeRegister(cpu->regs(), isa::Pep10::Register::SP, tmp, rw).completed);
    QVERIFY(mem->write(0x0000, {program.data(), program.size()}, rw).completed);
    QVERIFY(mem->write((quint16)isa::Pep10::MemoryVectors::SystemStackPtr, {osSP.data(), osSP.size()}, rw).completed);

    auto tick = cpu->tick(0);
    QCOMPARE(tick.error, sim::api::tick::Error::Success);

    QVERIFY(targets::pep10::isa::readRegister(cpu->regs(), isa::Pep10::Register::SP, tmp, rw).completed);
    QCOMPARE(tmp+10, 0x8086);
    QVERIFY(mem->read(tmp, {buf, sizeof(buf)}, rw).completed);

    for(int it=0; it<sizeof(truth); it++)
        QVERIFY2(buf[it] == truth[it], u"Mismatch at %1 with buf[%1]==%2 and truth[%1]==%3"_qs.arg(it).arg(buf[it],2,16).arg(truth[it],2,16).toStdString().data());
  }
};

#include "scall.moc"

QTEST_MAIN(ISA3Pep10_SCALL)
