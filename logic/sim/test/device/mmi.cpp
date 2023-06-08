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

#include "sim/device/broadcast/mmi.hpp"

auto desc = sim::api::device::Descriptor{
    .id = 1, .baseName = "cin", .fullName = "/cin"};

auto rw =
    sim::api::memory::Operation{.speculative = false,
                                .kind = sim::api::memory::Operation::Kind::data,
                                .effectful = true};
auto gs =
    sim::api::memory::Operation{.speculative = false,
                                .kind = sim::api::memory::Operation::Kind::data,
                                .effectful = false};

auto span = sim::api::memory::Target<quint16>::AddressSpan{.minOffset = 0,
                                                           .maxOffset = 0};

class SimDevice_MMI : public QObject {
  Q_OBJECT
private slots:
  void smoke() {
    QVERIFY_THROWS_NO_EXCEPTION(
        QSharedPointer<sim::memory::Input<quint16>>::create(desc, span, 0));
  }

  void read() {
    auto in =
        QSharedPointer<sim::memory::Input<quint16>>::create(desc, span, 0);
    auto endpoint = in->endpoint();
    endpoint->append_value(10);
    endpoint->append_value(20);
    quint8 tmp;
    // Read advances state
    QVERIFY(in->read(0, {&tmp, 1}, rw).completed);
    QCOMPARE(tmp, 10);
    // Get does not modify current value.
    QVERIFY(in->read(0, {&tmp, 1}, gs).completed);
    QCOMPARE(tmp, 10);
    // Read advances state
    QVERIFY(in->read(0, {&tmp, 1}, rw).completed);
    QCOMPARE(tmp, 20);
    // Out of MMI
    QCOMPARE(in->read(0, {&tmp, 1}, rw).error,
             sim::api::memory::Error::NeedsMMI);
    // Soft-fail MMI, should yield default value
    in->setFailPolicy(sim::api::memory::FailPolicy::YieldDefaultValue);
    QVERIFY(in->read(0, {&tmp, 1}, rw).completed);
    QCOMPARE(tmp, 0);
  }
  // TODO: Unread
};

#include "mmi.moc"

QTEST_MAIN(SimDevice_MMI)
