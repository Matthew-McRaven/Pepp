#include <QTest>
#include <QtCore>

#include "bits/operations/swap.hpp"
#include "sim/device/dense.hpp"
#include "targets/pep10/isa3/cpu.hpp"
#include "targets/pep10/isa3/helpers.hpp"
sim::api::memory::Operation rw = {.speculative = false,
                                  .kind =
                                      sim::api::memory::Operation::Kind::data,
                                  .effectful = false};

class Targets_ISA3Pep10_Bootstrap1 : public QObject {
  Q_OBJECT
private slots:
  void smoke() {
    sim::api::device::ID id = 0;
    auto nextID = [&id]() { return id++; };
    auto desc_mem = sim::api::device::Descriptor{.id = nextID(),
                                                 .compatible = nullptr,
                                                 .baseName = "dev",
                                                 .fullName = "/dev"};
    auto span = sim::api::memory::Target<quint16>::AddressSpan{
        .minOffset = 0, .maxOffset = 0xFFFF};
    sim::memory::Dense<quint16> mem(desc_mem, span, 0x08 /*nop*/);
    auto desc_cpu = sim::api::device::Descriptor{.id = nextID(),
                                                 .compatible = nullptr,
                                                 .baseName = "cpu",
                                                 .fullName = "/cpu"};
    targets::pep10::isa::CPU cpu(desc_cpu, nextID);
    cpu.setTarget(&mem);
    auto regs = cpu.regs();
    quint16 tmp = 0;

    using Register = isa::Pep10::Register;
    // Check that PC is incremented when executing NOP.
    QVERIFY(targets::pep10::isa::readRegister(regs, Register::PC, tmp, rw)
                .completed);
    QCOMPARE(tmp, 0);

    auto tick = cpu.tick(0);

    QVERIFY(targets::pep10::isa::readRegister(regs, Register::PC, tmp, rw)
                .completed);
    QCOMPARE(tmp, 1);

    // Check that A can be modified.
    quint8 v = 0b0001'0000; // NOTA
    QVERIFY(mem.write(0x01, &v, 1, rw).completed);
    QVERIFY(targets::pep10::isa::readRegister(regs, Register::A, tmp, rw)
                .completed);
    QCOMPARE(tmp, 0);

    tick = cpu.tick(1);

    QVERIFY(targets::pep10::isa::readRegister(regs, Register::A, tmp, rw)
                .completed);
    QCOMPARE(tmp, 0xFFFF);
  }
};

#include "bootstrap1.test.moc"

QTEST_MAIN(Targets_ISA3Pep10_Bootstrap1)
