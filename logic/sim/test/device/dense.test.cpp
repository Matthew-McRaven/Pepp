#include <QTest>
#include <QtCore>

#include "sim/device/dense.hpp"

static const sim::api::Memory::Operation op_rw{
    .speculative = false,
    .kind = sim::api::Memory::Operation::Kind::data,
    .effectful = true};

class SimDevice_Dense : public QObject {
  Q_OBJECT
private slots:

  void readwrite() {
    QFETCH(quint8, length);
    QFETCH(quint8, minOffset);
    // Hardcode arrays only go up to 8 for now, see `truth`.
    QVERIFY2(length <= 8, "Length must be less than 8.");

    auto desc = sim::api::Device::Descriptor{
        .id = 0, .compatible = nullptr, .baseName = "dev", .fullName = "/dev"};
    auto span = sim::api::Memory::Target<quint8>::AddressSpan{
        .minOffset = minOffset, .length = 255};
    sim::memory::Dense<quint8> dev(desc, span, 0xFE);

    quint64 reg = 0;
    quint8 *tmp = (quint8 *)&reg;
    auto ret = dev.read(0x10, tmp, length, op_rw);
    auto verify = [&ret]() {
      QVERIFY(ret.completed);
      QVERIFY(ret.advance);
      QVERIFY(!ret.pause);
      QVERIFY(!ret.sync);
      QCOMPARE(ret.error, sim::api::Memory::Error::Success);
    };
    auto compare = [&length](const quint8 *lhs, const quint8 *rhs) {
      if (lhs == nullptr || rhs == nullptr)
        return;
      for (int it = 0; it < length; it++)
        QCOMPARE(lhs[it], rhs[it]);
    };
    auto vec = QList<quint8>(length, 0xFE);
    verify();
    compare(vec.constData(), tmp);

    quint8 truth[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    memcpy(tmp, truth, length);
    ret = dev.write(0x10, tmp, length, op_rw);
    verify();
    compare(truth, tmp);
    // Check that data ends up in correct location in backing store.
    // i.e., the read API didn't do some awful bitmath it wasn't supposed to.
    compare(truth, dev.constData() + 0x10 - minOffset);
    ret = dev.read(0x10, tmp, 1, op_rw);
    verify();
    compare(truth, tmp);
  }

  void readwrite_data() {
    QTest::addColumn<quint8>("length");
    QTest::addColumn<quint8>("minOffset");
    QTest::addRow("length=1, minOffset=0") << quint8(1) << quint8(0);
    QTest::addRow("length=2, minOffset=0") << quint8(2) << quint8(0);
    QTest::addRow("length=4, minOffset=0") << quint8(4) << quint8(0);
    QTest::addRow("length=8, minOffset=0") << quint8(8) << quint8(0);
    QTest::addRow("length=1, minOffset=8") << quint8(1) << quint8(8);
    QTest::addRow("length=2, minOffset=8") << quint8(2) << quint8(8);
    QTest::addRow("length=4, minOffset=8") << quint8(4) << quint8(8);
    QTest::addRow("length=8, minOffset=8") << quint8(8) << quint8(8);
  }

  void oob() {
    auto desc = sim::api::Device::Descriptor{
        .id = 0, .compatible = nullptr, .baseName = "dev", .fullName = "/dev"};
    auto span = sim::api::Memory::Target<quint8>::AddressSpan{.minOffset = 0x10,
                                                              .length = 1};
    sim::memory::Dense<quint8> dev(desc, span, 0xFE);

    quint64 reg = 0;
    quint8 *tmp = (quint8 *)&reg;
    auto ret = dev.read(0x10, tmp, 1, op_rw);
    auto verify = [&ret](bool oob) {
      QVERIFY(oob ^ ret.completed);
      QVERIFY(oob ^ ret.advance);
      QVERIFY(!(oob ^ ret.pause));
      QVERIFY(!ret.sync);
      QCOMPARE(ret.error, oob ? sim::api::Memory::Error::OOBAccess
                              : sim::api::Memory::Error::Success);
    };
    verify(false);
    QCOMPARE(*tmp, 0xFE);

    *tmp = 0xca;
    ret = dev.read(0x9, tmp, 1, op_rw);
    verify(true);
    QCOMPARE(*tmp, 0xCA); // Read should not update tmp.
    ret = dev.read(0x11, tmp, 1, op_rw);
    verify(true);
    QCOMPARE(*tmp, 0xCA); // Read should not update tmp.

    // Neither write will stick, so tmp is meaningless
    *tmp = 0xfe;
    ret = dev.write(0x9, tmp, 1, op_rw);
    verify(true);
    ret = dev.write(0x11, tmp, 1, op_rw);
    verify(true);
  }
};

#include "dense.test.moc"

QTEST_MAIN(SimDevice_Dense)
