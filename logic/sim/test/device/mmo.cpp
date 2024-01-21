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

#include "sim/device/broadcast/mmo.hpp"

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

auto span = sim::api::memory::AddressSpan<quint16>{.minOffset = 0,
                                                   .maxOffset = 0};

class SimDevice_MMO : public QObject {
  Q_OBJECT
private slots:
  void smoke() {
    auto test= [](){auto x=QSharedPointer<sim::memory::Output<quint16>>::create(desc, span, 0);};
    QVERIFY_THROWS_NO_EXCEPTION(
         test();
    );
  }

  void write() {
    auto out =
        QSharedPointer<sim::memory::Output<quint16>>::create(desc, span, 0);
    auto endpoint = out->endpoint();
    quint8 tmp = 10;
    QVERIFY(out->write(0, {&tmp, 1}, rw).completed);
    tmp = 20;
    QVERIFY(out->write(0, {&tmp, 1}, rw).completed);
    auto _1 = endpoint->next_value();
    QVERIFY(_1.has_value());
    QCOMPARE(*_1, 10);
    auto _2 = endpoint->next_value();
    QVERIFY(_2.has_value());
    QCOMPARE(*_2, 20);
  }
  // TODO: Unwrite
};

#include "mmo.moc"

QTEST_MAIN(SimDevice_MMO)
