/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
 *
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

#include <QTest>
#include <QtCore>

#include "sim/device/dense.hpp"

static const sim::api::memory::Operation op_rw{
    .speculative = false,
    .kind = sim::api::memory::Operation::Kind::data,
    .effectful = true};

class SimDevice_Dense : public QObject {
  Q_OBJECT
private slots:

  void readwrite() {
    QFETCH(quint8, length);
    QFETCH(quint8, minOffset);
    // Hardcode arrays only go up to 8 for now, see `truth`.
    QVERIFY2(length <= 8, "Length must be less than 8.");

    auto desc = sim::api::device::Descriptor{
        .id = 0, .compatible = nullptr, .baseName = "dev", .fullName = "/dev"};
    auto span = sim::api::memory::AddressSpan<quint8>{
        .minOffset = minOffset, .maxOffset = 255};
    sim::memory::Dense<quint8> dev(desc, span, 0xFE);

    quint64 reg = 0;
    quint8 *tmp = (quint8 *)&reg;
    auto ret = dev.read(0x10, {tmp, length}, op_rw);
    auto verify = [&ret]() {
      QVERIFY(ret.completed);
      QVERIFY(!ret.pause);
      QCOMPARE(ret.error, sim::api::memory::Error::Success);
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
    ret = dev.write(0x10, {tmp, length}, op_rw);
    verify();
    compare(truth, tmp);
    // Check that data ends up in correct location in backing store.
    // i.e., the read API didn't do some awful bitmath it wasn't supposed to.
    compare(truth, dev.constData() + 0x10 - minOffset);
    ret = dev.read(0x10, {tmp, 1}, op_rw);
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
    auto desc = sim::api::device::Descriptor{
        .id = 0, .compatible = nullptr, .baseName = "dev", .fullName = "/dev"};
      auto span = sim::api::memory::AddressSpan<quint8>{
        .minOffset = 0x10, .maxOffset = 0x10};
    sim::memory::Dense<quint8> dev(desc, span, 0xFE);

    quint64 reg = 0;
    quint8 *tmp = (quint8 *)&reg;
    auto ret = dev.read(0x10, {tmp, 1}, op_rw);
    auto verify = [&ret](bool oob) {
      QVERIFY(oob ^ ret.completed);
      QVERIFY(!(oob ^ ret.pause));
      QCOMPARE(ret.error, oob ? sim::api::memory::Error::OOBAccess
                              : sim::api::memory::Error::Success);
    };
    verify(false);
    QCOMPARE(*tmp, 0xFE);

    *tmp = 0xca;
    ret = dev.read(0x9, {tmp, 1}, op_rw);
    verify(true);
    QCOMPARE(*tmp, 0xCA); // Read should not update tmp.
    ret = dev.read(0x11, {tmp, 1}, op_rw);
    verify(true);
    QCOMPARE(*tmp, 0xCA); // Read should not update tmp.

    // Neither write will stick, so tmp is meaningless
    *tmp = 0xfe;
    ret = dev.write(0x9, {tmp, 1}, op_rw);
    verify(true);
    ret = dev.write(0x11, {tmp, 1}, op_rw);
    verify(true);
  }
};

#include "dense_v1.moc"

QTEST_MAIN(SimDevice_Dense)
