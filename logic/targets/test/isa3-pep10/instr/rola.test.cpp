#include <QTest>
#include <QtCore>

#include "bits/operations/swap.hpp"
#include "sim/device/dense.hpp"
#include "targets/pep10/isa3/cpu.hpp"
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

class ISA3Pep10_ROLA : public QObject {
  Q_OBJECT
private slots:
  void u() {
    QFETCH(quint16, target);
    QFETCH(quint8, carry);
    auto [mem, cpu] = make();
    // Loop over a subset of possible values for the target register.
    quint16 tmp;

    // Can't capture CPU directly b/c structured bindings.
    auto _cpu = cpu;
    auto rreg = [&](isa::Pep10::Register reg) {
      quint16 tmp;
      _cpu->regs()->read(static_cast<quint16>(reg) * 2,
                         reinterpret_cast<quint8 *>(&tmp), 2, rw);
      return bits::hostOrder() != bits::Order::BigEndian ? bits::byteswap(tmp)
                                                         : tmp;
    };
    auto rcsr = [&](isa::Pep10::CSR csr) {
      quint8 tmp;
      _cpu->csrs()->read(static_cast<quint8>(csr), &tmp, 1, rw);
      return tmp;
    };

    static const auto target_reg = isa::Pep10::Register::A;
    auto endRegVal = static_cast<quint16>(target << 1) | (carry ? 1 : 0);

    // Object code for instruction under test.
    auto program = std::array<quint8, 1>{0x18};

    cpu->regs()->clear(0);
    cpu->csrs()->clear(0);
    cpu->csrs()->write(static_cast<quint8>(::isa::Pep10::CSR::C), &carry, 1,
                       rw);
    tmp = bits::hostOrder() != bits::Order::BigEndian ? bits::byteswap(target)
                                                      : target;
    cpu->regs()->write(static_cast<quint16>(target_reg) * 2,
                       reinterpret_cast<quint8 *>(&tmp), 2, rw);

    QVERIFY(mem->write(0, program.data(), program.size(), rw).completed);

    auto tick = cpu->tick(0);
    QCOMPARE(tick.error, sim::api::tick::Error::Success);

    tmp =
        bits::hostOrder() != bits::Order::BigEndian ? bits::byteswap(tmp) : tmp;
    QCOMPARE(rreg(isa::Pep10::Register::SP), 0);
    QCOMPARE(rreg(isa::Pep10::Register::X), 0);
    QCOMPARE(rreg(isa::Pep10::Register::TR), 0);
    QCOMPARE(rreg(isa::Pep10::Register::PC), 0x1);
    QCOMPARE(rreg(isa::Pep10::Register::IS), 0x18);
    // OS loaded the Mem[0x0001-0x0002].
    QCOMPARE(rreg(isa::Pep10::Register::OS), 0);
    // Check that target register had arithmetic performed.
    QCOMPARE(rreg(isa::Pep10::Register::A), endRegVal);
    // Check that target status bits match RTL.
    QCOMPARE(rcsr(isa::Pep10::CSR::N), 0);
    QCOMPARE(rcsr(isa::Pep10::CSR::Z), 0);
    QCOMPARE(rcsr(isa::Pep10::CSR::V), 0);
    auto new_A = rreg(target_reg);
    // Carry out if low order bit was non-zero
    QCOMPARE(rcsr(isa::Pep10::CSR::C), ((target & 0x8000) ? 1 : 0));
  }
  void u_data() {
    QTest::addColumn<quint16>("target");
    QTest::addColumn<quint8>("carry");
    QTest::addRow("0x0000, c=0") << quint16(0x0000) << quint8(0);
    QTest::addRow("0x0001, c=0") << quint16(0x0001) << quint8(0);
    QTest::addRow("0x7FFF, c=0") << quint16(0x7FFF) << quint8(0);
    QTest::addRow("0x8000, c=0") << quint16(0x8000) << quint8(0);
    QTest::addRow("0x8FFF, c=0") << quint16(0x8FFF) << quint8(0);
    QTest::addRow("0xFFFF, c=0") << quint16(0xFFFF) << quint8(0);
    QTest::addRow("0x0000, c=1") << quint16(0x0000) << quint8(1);
    QTest::addRow("0x0001, c=1") << quint16(0x0001) << quint8(1);
    QTest::addRow("0x7FFF, c=1") << quint16(0x7FFF) << quint8(1);
    QTest::addRow("0x8000, c=1") << quint16(0x8000) << quint8(1);
    QTest::addRow("0x8FFF, c=1") << quint16(0x8FFF) << quint8(1);
    QTest::addRow("0xFFFF, c=1") << quint16(0xFFFF) << quint8(1);
  }
};

#include "rola.test.moc"

QTEST_MAIN(ISA3Pep10_ROLA)
