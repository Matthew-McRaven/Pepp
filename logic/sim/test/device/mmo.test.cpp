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

auto span = sim::api::memory::Target<quint16>::AddressSpan{.minOffset = 0,
                                                           .maxOffset = 0};

class SimDevice_MMO : public QObject {
  Q_OBJECT
private slots:
  void smoke() {
    QVERIFY_THROWS_NO_EXCEPTION(
        QSharedPointer<sim::memory::Output<quint16>>::create(desc, span, 0));
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

#include "mmo.test.moc"

QTEST_MAIN(SimDevice_MMO)
