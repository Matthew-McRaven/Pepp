#include <QTest>
#include <QtCore>

#include "sim/device/dense.hpp"
#include "targets/pep10/isa3/cpu.hpp"
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

    // Check that PC is incremented when executing NOP.
    regs->read(static_cast<quint8>(isa::Pep10::Register::PC) * 2,
               reinterpret_cast<quint8 *>(&tmp), 2, rw);
    QCOMPARE(tmp, 0);

    auto tick = cpu.tick(0);

    regs->read(static_cast<quint8>(isa::Pep10::Register::PC) * 2,
               reinterpret_cast<quint8 *>(&tmp), 2, rw);
    QCOMPARE(tmp, 1);

    // Check that A can be modified.
    quint8 v = 0b0001'0000;
    QVERIFY(mem.write(0x01, &v, 1, rw).completed);
    regs->read(static_cast<quint8>(isa::Pep10::Register::A) * 2,
               reinterpret_cast<quint8 *>(&tmp), 2, rw);
    QCOMPARE(tmp, 0);
    tick = cpu.tick(1);
    regs->read(static_cast<quint8>(isa::Pep10::Register::A) * 2,
               reinterpret_cast<quint8 *>(&tmp), 2, rw);
    QCOMPARE(tmp, 0xFFFF);
  }
};

#include "bootstrap1.test.moc"

QTEST_MAIN(Targets_ISA3Pep10_Bootstrap1)
