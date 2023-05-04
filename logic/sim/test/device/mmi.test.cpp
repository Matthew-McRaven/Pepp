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
    auto in =
        QSharedPointer<sim::memory::Input<quint16>>::create(desc, span, 0);
  }

  void read() {
    auto in =
        QSharedPointer<sim::memory::Input<quint16>>::create(desc, span, 0);
    auto endpoint = in->endpoint();
    endpoint->append_value(10);
    endpoint->append_value(20);
    quint8 tmp;
    // Read advances state
    QVERIFY(in->read(0, &tmp, 1, rw).completed);
    QCOMPARE(tmp, 10);
    // Get does not
    QVERIFY(in->read(0, &tmp, 1, gs).completed);
    QCOMPARE(tmp, 10);
    // Read advances state
    QVERIFY(in->read(0, &tmp, 1, rw).completed);
    QCOMPARE(tmp, 20);
    // Out of MMI
    QCOMPARE(in->read(0, &tmp, 1, rw).error, sim::api::memory::Error::NeedsMMI);
  }
};

#include "mmi.test.moc"

QTEST_MAIN(SimDevice_MMI)
