#include <QTest>
#include <QtCore>

#include "sim/device/dense.hpp"
#include "sim/device/simple_bus.hpp"
auto rw =
    sim::api::memory::Operation{.speculative = false,
                                .kind = sim::api::memory::Operation::Kind::data,
                                .effectful = true};

auto d1 = sim::api::device::Descriptor{
    .id = 1, .baseName = "d1", .fullName = "/bus0/d1"};
auto d2 = sim::api::device::Descriptor{
    .id = 2, .baseName = "d2", .fullName = "/bus0/d2"};
auto d3 = sim::api::device::Descriptor{
    .id = 3, .baseName = "d3", .fullName = "/bus0/d3"};
auto b1 = sim::api::device::Descriptor{
    .id = 4, .baseName = "bus0", .fullName = "/bus0"};
using Span = sim::api::memory::Target<quint16>::AddressSpan;
auto make = []() {
  auto m1 = QSharedPointer<sim::memory::Dense<quint16>>::create(
      d1, Span{.minOffset = 0, .maxOffset = 0x1});
  auto m2 = QSharedPointer<sim::memory::Dense<quint16>>::create(
      d2, Span{.minOffset = 0, .maxOffset = 0x1});
  auto m3 = QSharedPointer<sim::memory::Dense<quint16>>::create(
      d3, Span{.minOffset = 0, .maxOffset = 0x1});
  auto bus = QSharedPointer<sim::memory::SimpleBus<quint16>>::create(
      b1, Span{.minOffset = 0, .maxOffset = 5});
  bus->pushFrontTarget(Span{.minOffset = 0, .maxOffset = 1}, &*m1);
  bus->pushFrontTarget(Span{.minOffset = 2, .maxOffset = 3}, &*m2);
  bus->pushFrontTarget(Span{.minOffset = 4, .maxOffset = 5}, &*m3);
  return std::tuple{bus, m1, m2, m3};
};

class SimDevice_SimpleBus : public QObject {
  Q_OBJECT
private slots:
  void individual_access() {
    auto [bus, m1, m2, m3] = make();
    sim::api::memory::Target<quint16> *memArr[3] = {&*m1, &*m2, &*m3};
    quint8 buf[2];
    bits::memclr(buf, sizeof(buf) * sizeof(*buf));

    // Can write to each individual memory and read on bus.
    for (int i = 0; i < 2; i++) {
      auto m = memArr[i];
      bits::memcpy_endian(buf, bits::Order::BigEndian, 2, quint16(0x0001));
      QVERIFY(m->write(0, buf, 2, rw).completed);
      bits::memclr(buf, 2);
      QVERIFY(bus->read(0 + i * 2, buf, 2, rw).completed);
      for (int j = 0; j < 1; j++)
        QCOMPARE(buf[j], j);
    }
  };

  void group_access() {
    auto [bus, m1, m2, m3] = make();
    sim::api::memory::Target<quint16> *memArr[3] = {&*m1, &*m2, &*m3};
    quint8 buf[6];
    for (int it = 0; it < 6; it++)
      buf[it] = it;
    QVERIFY(bus->write(0, buf, sizeof(buf), rw).completed);
    bits::memclr(buf, sizeof(buf) * sizeof(*buf));

    // Can write to bus and read each individual memory.
    for (int i = 0; i < 2; i++) {
      auto m = memArr[i];
      QVERIFY(m->read(0, buf, 2, rw).completed);
      for (int j = 0; j < 1; j++)
        QCOMPARE(buf[j], i * 2 + j);
    }
  };

  void dump() {
    auto [bus, m1, m2, m3] = make();
    sim::api::memory::Target<quint16> *memArr[3] = {&*m1, &*m2, &*m3};
    quint8 buf[6];
    for (int it = 0; it < 6; it++)
      buf[it] = it;
    QVERIFY(bus->write(0, buf, sizeof(buf), rw).completed);
    bits::memclr(buf, sizeof(buf) * sizeof(*buf));

    bus->dump(buf, sizeof(buf));

    for (int it = 0; it < 6; it++)
      QCOMPARE(buf[it], it);
  };

  void dump_sparse() {
    auto m1 = QSharedPointer<sim::memory::Dense<quint16>>::create(
        d1, Span{.minOffset = 0, .maxOffset = 0x1});
    auto m2 = QSharedPointer<sim::memory::Dense<quint16>>::create(
        d2, Span{.minOffset = 0, .maxOffset = 0x1});
    auto m3 = QSharedPointer<sim::memory::Dense<quint16>>::create(
        d3, Span{.minOffset = 0, .maxOffset = 0x1});
    auto bus = QSharedPointer<sim::memory::SimpleBus<quint16>>::create(
        b1, Span{.minOffset = 0, .maxOffset = 9});
    bus->pushFrontTarget(Span{.minOffset = 0, .maxOffset = 1}, &*m1);
    bus->pushFrontTarget(Span{.minOffset = 4, .maxOffset = 5}, &*m2);
    bus->pushFrontTarget(Span{.minOffset = 8, .maxOffset = 9}, &*m3);

    quint8 buf[10], out[10];
    bits::memclr(buf, sizeof(buf));
    bits::memclr(out, sizeof(out));
    bits::memcpy_endian(buf + 0, bits::Order::BigEndian, 2, quint16(0x0001));
    m1->write(0, buf + 0, 2, rw);
    bits::memcpy_endian(buf + 4, bits::Order::BigEndian, 2, quint16(0x0405));
    m2->write(0, buf + 4, 2, rw);
    bits::memcpy_endian(buf + 8, bits::Order::BigEndian, 2, quint16(0x0809));
    m3->write(0, buf + 8, 2, rw);

    bus->dump(out, sizeof(out));
    for (int it = 0; it < sizeof(out); it++)
      QCOMPARE(buf[it], out[it]);
  }
};

#include "simple_bus.test.moc"

QTEST_MAIN(SimDevice_SimpleBus);