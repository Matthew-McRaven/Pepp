#include "sim3/trace/modified.hpp"
#include <catch.hpp>
#include "sim3/subsystems/bus/simple.hpp"
#include "sim3/subsystems/ram/dense.hpp"
#include "sim3/trace/buffers/infinite.hpp"

using namespace sim::trace2;
using I = Interval<uint16_t>;
I i0 = {0, 1}, i1 = {1, 2}, i2 = {0, 2}, i3{2, 3};
TEST_CASE("Interval", "[scope:sim][kind:unit][arch:*]") {
  SECTION("Ordering") {
    CHECK(!(i0 < i0));
    CHECK(i0 == i0);
    CHECK(i0 < i1);
    CHECK(i0 != i1);
    CHECK(i0 < i2);
    CHECK(i0 != i2);
    CHECK(i0 < i3);
    CHECK(i0 != i3);

    CHECK(i1 > i0);
    CHECK(i1 != i0);
    CHECK(!(i1 < i1));
    CHECK(i1 == i1);
    CHECK(i1 > i2);
    CHECK(i1 != i2);
    CHECK(i1 < i3);
    CHECK(i0 != i3);

    CHECK(i2 > i0);
    CHECK(i2 != i0);
    CHECK(i2 < i1);
    CHECK(i2 != i1);
    CHECK(!(i2 < i2));
    CHECK(i2 == i2);
    CHECK(i2 < i3);
    CHECK(i2 != i3);

    CHECK(i3 > i0);
    CHECK(i3 != i0);
    CHECK(i3 > i1);
    CHECK(i3 != i1);
    CHECK(i3 > i2);
    CHECK(i3 != i2);
    CHECK(!(i3 < i3));
    CHECK(i3 == i3);
  }
  SECTION("Intersection") {
    CHECK(intersects(i0, i0));
    CHECK(intersects(i0, i1));
    CHECK(intersects(i0, i2));
    CHECK(!intersects(i0, i3));

    CHECK(intersects(i1, i0));
    CHECK(intersects(i1, i1));
    CHECK(intersects(i1, i2));
    CHECK(intersects(i1, i3));

    CHECK(intersects(i2, i0));
    CHECK(intersects(i2, i1));
    CHECK(intersects(i2, i2));
    CHECK(intersects(i2, i3));

    CHECK(!intersects(i3, i0));
    CHECK(intersects(i3, i1));
    CHECK(intersects(i3, i2));
    CHECK(intersects(i3, i3));
  }
  SECTION("Containment") {

    CHECK(contains(i0, i0));
    CHECK(!contains(i0, i1));
    CHECK(!contains(i0, i2));
    CHECK(!contains(i0, i3));

    CHECK(!contains(i1, i0));
    CHECK(contains(i1, i1));
    CHECK(!contains(i1, i2));
    CHECK(!contains(i1, i3));

    CHECK(contains(i2, i0));
    CHECK(contains(i2, i1));
    CHECK(contains(i2, i2));
    CHECK(!contains(i2, i3));

    CHECK(!contains(i3, i0));
    CHECK(!contains(i3, i1));
    CHECK(!contains(i3, i2));
    CHECK(contains(i3, i3));
  }
  SECTION("Wrap-around") {
    I a = {1, 0xffff}, b = {0, 1};
    REQUIRE(intersects(a, b));
    I i = intersection(a, b);
    CHECK(i.lower() == 0);
    CHECK(i.upper() == 0xffff);
  }
}
using IS = IntervalSet<uint16_t, true>;
TEST_CASE("Inclusive IntervalSet", "[scope:sim][kind:unit][arch:*]") {
  SECTION("Append-only, no merge") {
    IS set;
    set.insert({0, 0});
    CHECK(set.intervals().size() == 1);
    set.insert({2, 2});
    CHECK(set.intervals().size() == 2);
    set.insert({4, 4});
    CHECK(set.intervals().size() == 3);
  }
  SECTION("Clear") {
    IS set;
    set.insert({0, 0});
    set.insert({2, 2});
    set.insert({4, 4});
    REQUIRE(set.intervals().size() == 3);
    set.clear();
    CHECK(set.intervals().empty());
  }
  SECTION("Append-only and merge") {
    IS set;
    set.insert({0, 1});
    CHECK(set.intervals().size() == 1);
    set.insert({2, 3});
    CHECK(set.intervals().size() == 1);
    set.insert({4, 4});
    CHECK(set.intervals().size() == 1);
  }
  SECTION("Prepend-only, no merge") {
    IS set;
    set.insert({4, 4});
    CHECK(set.intervals().size() == 1);
    set.insert({2, 2});
    CHECK(set.intervals().size() == 2);
    set.insert({0, 0});
    CHECK(set.intervals().size() == 3);
  }
  SECTION("Prepend-only and merge") {
    IS set;
    set.insert({4, 4});
    CHECK(set.intervals().size() == 1);
    set.insert({2, 3});
    CHECK(set.intervals().size() == 1);
    set.insert({0, 1});
    CHECK(set.intervals().size() == 1);
  }
  SECTION("Merge previous and next intervals") {
    IS set;
    set.insert({0, 1});
    set.insert({3, 4});
    CHECK(set.intervals().size() == 2);
    set.insert({2, 4});
    CHECK(set.intervals().size() == 1);
  }
  SECTION("Insert contained in existing interval") {
    IS set;
    set.insert({0, 1});
    CHECK(set.intervals().size() == 1);
    set.insert(1);
    CHECK(set.intervals().size() == 1);
  }
  SECTION("Existing contained in inserted interval") {
    IS set;
    set.insert({0, 0});
    set.insert({2, 2});
    set.insert({4, 4});
    set.insert({6, 6});
    CHECK(set.intervals().size() == 4);
    set.insert({1, 5});
    CHECK(set.intervals().size() == 1);
  }
  SECTION("Wrap-around") {
    IS set;
    set.insert({0, 0xFFFD});
    CHECK(set.intervals().size() == 1);
    set.insert({0xFFFE, 0xFFFF});
    CHECK(set.intervals().size() == 1);
    auto i = *set.intervals().begin();
    CHECK(i.lower() == 0);
    CHECK(i.upper() == 0xffff);
  }
}

using ISE = IntervalSet<uint16_t, false>;
TEST_CASE("Exclusive IntervalSet", "[scope:sim][kind:unit][arch:*]") {
  SECTION("Append-only, no merge") {
    ISE set;
    set.insert({0, 1});
    CHECK(set.intervals().size() == 1);
    set.insert({2, 3});
    CHECK(set.intervals().size() == 2);
    set.insert({4, 5});
    CHECK(set.intervals().size() == 3);
  }
  SECTION("Clear") {
    ISE set;
    set.insert({0, 0});
    set.insert({2, 2});
    set.insert({4, 4});
    REQUIRE(set.intervals().size() == 3);
    set.clear();
    CHECK(set.intervals().empty());
  }
  SECTION("Append-only and merge") {
    ISE set;
    set.insert({0, 1});
    CHECK(set.intervals().size() == 1);
    set.insert({1, 2});
    CHECK(set.intervals().size() == 1);
    set.insert({2, 3});
    CHECK(set.intervals().size() == 1);
  }
  SECTION("Prepend-only, no merge") {
    ISE set;
    set.insert({4, 5});
    CHECK(set.intervals().size() == 1);
    set.insert({2, 3});
    CHECK(set.intervals().size() == 2);
    set.insert({0, 1});
    CHECK(set.intervals().size() == 3);
  }
  SECTION("Prepend-only and merge") {
    ISE set;
    set.insert({3, 4});
    CHECK(set.intervals().size() == 1);
    set.insert({2, 3});
    CHECK(set.intervals().size() == 1);
    set.insert({0, 2});
    CHECK(set.intervals().size() == 1);
  }
  SECTION("Merge next intervals") {
    ISE set;
    set.insert({0, 1});
    set.insert({3, 4});
    CHECK(set.intervals().size() == 2);
    set.insert({2, 4});
    CHECK(set.intervals().size() == 2);
  }
  SECTION("Insert contained in existing interval") {
    ISE set;
    set.insert({0, 1});
    CHECK(set.intervals().size() == 1);
    set.insert(1);
    CHECK(set.intervals().size() == 1);
  }
  SECTION("Existing contained in inserted interval") {
    ISE set;
    set.insert({0, 0});

    set.insert({2, 2});
    set.insert({4, 4});

    set.insert({6, 6});
    CHECK(set.intervals().size() == 4);
    set.insert({1, 5});
    CHECK(set.intervals().size() == 3);
  }
  SECTION("Wrap-around") {
    ISE set;
    set.insert({0, 0xFFFD});
    CHECK(set.intervals().size() == 1);
    set.insert({0xFFFD, 0xFFFF});
    CHECK(set.intervals().size() == 1);
    auto i = *set.intervals().begin();
    CHECK(i.lower() == 0);
    CHECK(i.upper() == 0xffff);
  }
}

TEST_CASE("AddressBiMap", "[scope:sim][kind:unit][arch:*]") {
  using namespace sim::trace2;
  using I = Interval<uint16_t>;
  using Map = AddressBiMap<uint16_t, uint16_t>;

  SECTION("Adjacent does not merge") {
    Map m;
    m.insert_or_overwrite(I{0, 9}, I{100, 109}, 0, 0);
    m.insert_or_overwrite(I{10, 19}, I{100, 109}, 1, 0);
    m.insert_or_overwrite(I{20, 29}, I{100, 109}, 2, 0);
    CHECK(m.regions().size() == 3);
    for (int it = 0; it < 30; it++) {
      auto region = m.region_at(it);
      CHECK(region.has_value());
      CHECK(region->device == it / 10);
    }
  }

  SECTION("Delete overlapped element") {
    Map m;
    m.insert_or_overwrite(I{0, 9}, I{100, 109}, 0, 0);
    m.insert_or_overwrite(I{10, 19}, I{100, 109}, 1, 0);
    m.insert_or_overwrite(I{20, 29}, I{100, 109}, 2, 0);
    // Replace a region without affecting adjacent regions.
    m.insert_or_overwrite(I{10, 19}, I{100, 109}, 3, 0);
    CHECK(m.regions().size() == 3);
    CHECK(m.region_at(9)->device == 0);
    for (int it = 10; it < 19; it++) {
      CHECK(m.region_at(it)->device == 3);
    }
    CHECK(m.region_at(20)->device == 2);
  }

  SECTION("Contract adjacent regions") {
    Map m;
    m.insert_or_overwrite(I{0, 9}, I{100, 109}, 0, 0);
    m.insert_or_overwrite(I{10, 19}, I{100, 109}, 1, 0);
    m.insert_or_overwrite(I{20, 29}, I{100, 109}, 2, 0);
    // Contract adjacent regions
    m.insert_or_overwrite(I{5, 15}, I{105, 115}, 3, 0);
    CHECK(m.regions().size() == 4);
    CHECK(m.region_at(4)->device == 0);
    for (int it = 5; it < 15; it++) {
      CHECK(m.region_at(it)->device == 3);
    }
    CHECK(m.region_at(16)->device == 1);
    // Check that "to" address ranges get shrunk accordingly.
    for (auto reg : m.regions())
      CHECK(size(reg.from) == size(reg.to));
  }

  SECTION("Forward translation") {
    Map m;
    m.insert_or_overwrite(I{0, 2}, I{100, 102}, 0, 0);
    m.insert_or_overwrite(I{3, 5}, I{100, 102}, 1, 0);
    m.insert_or_overwrite(I{6, 8}, I{100, 102}, 2, 0);
    for (int it = 0; it < 9; it++) {
      auto [success, id, addr] = m.value(it);
      REQUIRE(success);
      CHECK(id == it / 3);
      CHECK(addr == 100 + (it % 3));
    }
    auto [success, id, addr] = m.value(10);
    CHECK_FALSE(success);
  }

  SECTION("Backward translation") {
    Map m;
    m.insert_or_overwrite(I{0, 2}, I{100, 102}, 0, 0);
    m.insert_or_overwrite(I{3, 5}, I{100, 102}, 1, 0);
    m.insert_or_overwrite(I{6, 8}, I{100, 102}, 2, 0);
    for (int dev_id = 0; dev_id < 3; dev_id++) {
      for (int dev_addr = 100; dev_addr < 103; dev_addr++) {
        auto [success, addr] = m.key(dev_id, dev_addr);
        REQUIRE(success);
        CHECK(addr == (dev_addr - 100) + (dev_id * 3));
      }
    }
    CHECK_FALSE(std::get<0>(m.key(2, 104)));
    CHECK_FALSE(std::get<0>(m.key(2, 99)));
    CHECK_FALSE(std::get<0>(m.key(3, 0)));
    CHECK_FALSE(std::get<0>(m.key(3, 99)));
  }
}

TEST_CASE("ModifiedAddressSink", "[scope:sim][kind:unit][arch:*]") {
  using namespace sim::api2::packet;
  quint8 v1[] = {0};
  quint8 v2[] = {0, 0};
  SECTION("Append-only, no merge") {
    InfiniteBuffer buf;
    buf.trace(0, true);
    ModifiedAddressSink<uint16_t> sink;
    buf.emitFrameStart();
    buf.emitWrite<quint16>(0, 0u, v1, v1);
    buf.emitWrite<quint16>(0, 2u, v1, v1);
    buf.emitWrite<quint16>(0, 4u, v1, v1);
    buf.emitFrameStart();
    auto frame = buf.cbegin();
    for (auto pkt = frame.cbegin(); pkt != frame.cend(); ++pkt)
      sink.analyze(pkt, sim::api2::trace::Direction::Forward);
    CHECK(sink.intervals().size() == 3);
    for (int it = 0; it <= 5; it++)
      CHECK(sink.contains(it) == (it % 2 == 0));
    sink.clear();
    CHECK(sink.intervals().size() == 0);
  }
  SECTION("Append-only and merge") {
    InfiniteBuffer buf;
    buf.trace(0, true);
    ModifiedAddressSink<uint16_t> sink;
    buf.emitFrameStart();
    buf.emitWrite<quint16>(0, 0u, v2, v2);
    buf.emitWrite<quint16>(0, 2u, v2, v2);
    buf.emitWrite<quint16>(0, 4u, v2, v2);
    buf.emitFrameStart();
    auto frame = buf.cbegin();
    for (auto pkt = frame.cbegin(); pkt != frame.cend(); ++pkt)
      sink.analyze(pkt, sim::api2::trace::Direction::Forward);
    CHECK(sink.intervals().size() == 1);
    for (int it = 0; it <= 5; it++)
      CHECK(sink.contains(it));
  }
  SECTION("Prepend-only, no merge") {
    InfiniteBuffer buf;
    buf.trace(0, true);
    ModifiedAddressSink<uint16_t> sink;
    buf.emitFrameStart();
    buf.emitWrite<quint16>(0, 4u, v1, v1);
    buf.emitWrite<quint16>(0, 2u, v1, v1);
    buf.emitWrite<quint16>(0, 0, v1, v1);
    buf.emitFrameStart();
    auto frame = buf.cbegin();
    for (auto pkt = frame.cbegin(); pkt != frame.cend(); ++pkt)
      sink.analyze(pkt, sim::api2::trace::Direction::Forward);
    CHECK(sink.intervals().size() == 3);
    for (int it = 0; it <= 5; it++)
      CHECK(sink.contains(it) == (it % 2 == 0));
  }
  SECTION("Prepend-only and merge") {
    InfiniteBuffer buf;
    buf.trace(0, true);
    ModifiedAddressSink<uint16_t> sink;
    buf.emitFrameStart();
    buf.emitWrite<quint16>(0, 4, v2, v2);
    buf.emitWrite<quint16>(0, 2, v2, v2);
    buf.emitWrite<quint16>(0, 0, v2, v2);
    buf.emitFrameStart();
    auto frame = buf.cbegin();
    for (auto pkt = frame.cbegin(); pkt != frame.cend(); ++pkt)
      sink.analyze(pkt, sim::api2::trace::Direction::Forward);
    CHECK(sink.intervals().size() == 1);
    for (int it = 0; it <= 5; it++)
      CHECK(sink.contains(it));
  }
  SECTION("Merge previous and next intervals") {
    InfiniteBuffer buf;
    buf.trace(0, true);
    ModifiedAddressSink<uint16_t> sink;
    buf.emitFrameStart();
    buf.emitWrite<quint16>(0, 0, v2, v2);
    buf.emitWrite<quint16>(0, 4, v2, v2);
    buf.emitWrite<quint16>(0, 2, v2, v2);
    buf.emitFrameStart();
    auto frame = buf.cbegin();
    for (auto pkt = frame.cbegin(); pkt != frame.cend(); ++pkt)
      sink.analyze(pkt, sim::api2::trace::Direction::Forward);
    CHECK(sink.intervals().size() == 1);
    for (int it = 0; it <= 5; it++)
      CHECK(sink.contains(it));
  }
  SECTION("Insert contained in existing interval") {
    InfiniteBuffer buf;
    buf.trace(0, true);
    ModifiedAddressSink<uint16_t> sink;
    buf.emitFrameStart();
    buf.emitWrite<quint16>(0, 0, v2, v2);
    buf.emitWrite<quint16>(0, 0, v1, v1);
    buf.emitFrameStart();
    auto frame = buf.cbegin();
    for (auto pkt = frame.cbegin(); pkt != frame.cend(); ++pkt)
      sink.analyze(pkt, sim::api2::trace::Direction::Forward);
    CHECK(sink.intervals().size() == 1);
    CHECK(sink.contains(0));
    CHECK(sink.contains(1));
  }
  SECTION("Existing contained in inserted interval") {
    InfiniteBuffer buf;
    buf.trace(0, true);
    ModifiedAddressSink<uint16_t> sink;
    buf.emitFrameStart();
    buf.emitWrite<quint16>(0, 0, v1, v1);
    buf.emitWrite<quint16>(0, 2, v1, v1);
    buf.emitWrite<quint16>(0, 4, v1, v1);
    buf.emitWrite<quint16>(0, 6, v1, v1);
    buf.emitFrameStart();
    auto frame = buf.cbegin();
    for (auto pkt = frame.cbegin(); pkt != frame.cend(); ++pkt)
      sink.analyze(pkt, sim::api2::trace::Direction::Forward);
    CHECK(sink.intervals().size() == 4);
    for (int it = 0; it <= 7; it++)
      CHECK(sink.contains(it) == (it % 2 == 0));
    buf.clear();
    quint8 v5[] = {1, 2, 3, 4, 5};
    buf.emitFrameStart();
    buf.emitWrite<quint16>(0, 1, v5, v5);
    buf.emitFrameStart();
    frame = buf.cbegin();
    for (auto pkt = frame.cbegin(); pkt != frame.cend(); ++pkt)
      sink.analyze(pkt, sim::api2::trace::Direction::Forward);
    CHECK(sink.intervals().size() == 1);
    for (int it = 0; it <= 6; it++)
      CHECK(sink.contains(it));
    CHECK_FALSE(sink.contains(7));
  }
  SECTION("Wrap-around") {
    InfiniteBuffer buf;
    buf.trace(0, true);
    ModifiedAddressSink<uint16_t> sink;
    buf.emitFrameStart();
    buf.emitWrite<quint16>(0, 0xFFFF, v2, v2);
    buf.emitFrameStart();
    auto frame = buf.cbegin();
    for (auto pkt = frame.cbegin(); pkt != frame.cend(); ++pkt)
      sink.analyze(pkt, sim::api2::trace::Direction::Forward);
    CHECK(sink.intervals().size() == 2);
    CHECK_FALSE(sink.contains(0xFFFE));
    CHECK(sink.contains(0xFFFF));
    CHECK(sink.contains(0x0000));
    CHECK_FALSE(sink.contains(0x0001));
    // Check there's not some overflow to negative max.
    CHECK_FALSE(sink.contains(0x8000));
  }
}

TEST_CASE("TranslatingModifiedAddressSink", "[scope:sim][kind:unit][arch:*]") {
  using namespace sim::api2::packet;
  auto d1 = sim::api2::device::Descriptor{.id = 1, .baseName = "d1", .fullName = "/bus0/d1"};
  auto d2 = sim::api2::device::Descriptor{.id = 2, .baseName = "d2", .fullName = "/bus0/d2"};
  auto d3 = sim::api2::device::Descriptor{.id = 3, .baseName = "d3", .fullName = "/bus0/d3"};
  auto b1 = sim::api2::device::Descriptor{.id = 4, .baseName = "bus0", .fullName = "/bus0"};
  auto b2 = sim::api2::device::Descriptor{.id = 5, .baseName = "bus1", .fullName = "/bus1"};
  using Span = sim::api2::memory::AddressSpan<quint16>;
  auto make = [&](sim::api2::trace::Buffer *tb) {
    auto m1 = QSharedPointer<sim::memory::Dense<quint16>>::create(d1, Span(0, 1));
    auto m2 = QSharedPointer<sim::memory::Dense<quint16>>::create(d2, Span(0, 1));
    auto m3 = QSharedPointer<sim::memory::Dense<quint16>>::create(d3, Span(0, 1));
    auto bus = QSharedPointer<sim::memory::SimpleBus<quint16>>::create(b1, Span(0, 5));
    CHECK(bus->deviceID() == b1.id);
    bus->pushFrontTarget(Span(0, 1), &*m1);
    bus->pushFrontTarget(Span(2, 3), &*m2);
    bus->pushFrontTarget(Span(4, 5), &*m3);
    m1->setBuffer(tb);
    m1->trace(true);
    m2->setBuffer(tb);
    m2->trace(true);
    m3->setBuffer(tb);
    m3->trace(true);
    bus->setBuffer(tb);
    bus->trace(true);
    return std::tuple{bus, m1, m2, m3};
  };
  quint8 v1[] = {0};
  quint8 v2[] = {0, 0};
  SECTION("Append-only, no merge") {
    InfiniteBuffer buf;
    auto [bus, m1, m2, m3] = make(&buf);
    auto paths = QSharedPointer<sim::api2::Paths>::create();
    auto path = paths->add(0, bus->deviceID());
    TranslatingModifiedAddressSink<uint16_t> sink(paths, &*bus);
    auto guard = sim::api2::trace::PathGuard(&buf, path);
    buf.emitFrameStart();
    buf.emitWrite<quint16>(d1.id, 0u, v1, v1);
    buf.emitWrite<quint16>(d2.id, 0u, v1, v1);
    buf.emitWrite<quint16>(d3.id, 0u, v1, v1);
    buf.emitFrameStart();
    auto frame = buf.cbegin();
    for (auto pkt = frame.cbegin(); pkt != frame.cend(); ++pkt)
      sink.analyze(pkt, sim::api2::trace::Direction::Forward);
    CHECK(sink.intervals().size() == 3);
    for (int it = 0; it <= 5; it++)
      CHECK(sink.contains(it) == (it % 2 == 0));
    sink.clear();
    CHECK(sink.intervals().size() == 0);
  }
  SECTION("Append-only and merge") {
    InfiniteBuffer buf;
    auto [bus, m1, m2, m3] = make(&buf);
    auto paths = QSharedPointer<sim::api2::Paths>::create();
    auto path = paths->add(0, bus->deviceID());
    TranslatingModifiedAddressSink<uint16_t> sink(paths, &*bus);
    auto guard = sim::api2::trace::PathGuard(&buf, path);
    buf.emitFrameStart();
    buf.emitWrite<quint16>(d1.id, 0u, v2, v2);
    buf.emitWrite<quint16>(d2.id, 0u, v2, v2);
    buf.emitWrite<quint16>(d3.id, 0u, v2, v2);
    buf.emitFrameStart();
    auto frame = buf.cbegin();
    for (auto pkt = frame.cbegin(); pkt != frame.cend(); ++pkt)
      sink.analyze(pkt, sim::api2::trace::Direction::Forward);
    CHECK(sink.intervals().size() == 1);
    for (int it = 0; it <= 5; it++)
      CHECK(sink.contains(it));
  }
  SECTION("Prepend-only, no merge") {
    InfiniteBuffer buf;
    auto [bus, m1, m2, m3] = make(&buf);
    auto paths = QSharedPointer<sim::api2::Paths>::create();
    auto path = paths->add(0, bus->deviceID());
    TranslatingModifiedAddressSink<uint16_t> sink(paths, &*bus);
    auto guard = sim::api2::trace::PathGuard(&buf, path);
    buf.emitFrameStart();
    buf.emitWrite<quint16>(d3.id, 0u, v1, v1);
    buf.emitWrite<quint16>(d2.id, 0u, v1, v1);
    buf.emitWrite<quint16>(d1.id, 0u, v1, v1);
    buf.emitFrameStart();
    auto frame = buf.cbegin();
    for (auto pkt = frame.cbegin(); pkt != frame.cend(); ++pkt)
      sink.analyze(pkt, sim::api2::trace::Direction::Forward);
    CHECK(sink.intervals().size() == 3);
    for (int it = 0; it <= 5; it++)
      CHECK(sink.contains(it) == (it % 2 == 0));
  }
  SECTION("Prepend-only and merge") {
    InfiniteBuffer buf;
    auto [bus, m1, m2, m3] = make(&buf);
    auto paths = QSharedPointer<sim::api2::Paths>::create();
    auto path = paths->add(0, bus->deviceID());
    TranslatingModifiedAddressSink<uint16_t> sink(paths, &*bus);
    auto guard = sim::api2::trace::PathGuard(&buf, path);
    buf.emitFrameStart();
    buf.emitWrite<quint16>(d3.id, 0, v2, v2);
    buf.emitWrite<quint16>(d2.id, 0, v2, v2);
    buf.emitWrite<quint16>(d1.id, 0, v2, v2);
    buf.emitFrameStart();
    auto frame = buf.cbegin();
    for (auto pkt = frame.cbegin(); pkt != frame.cend(); ++pkt)
      sink.analyze(pkt, sim::api2::trace::Direction::Forward);
    CHECK(sink.intervals().size() == 1);
    for (int it = 0; it <= 5; it++)
      CHECK(sink.contains(it));
  }
  SECTION("Merge previous and next intervals") {
    InfiniteBuffer buf;
    auto [bus, m1, m2, m3] = make(&buf);
    auto paths = QSharedPointer<sim::api2::Paths>::create();
    auto path = paths->add(0, bus->deviceID());
    TranslatingModifiedAddressSink<uint16_t> sink(paths, &*bus);
    auto guard = sim::api2::trace::PathGuard(&buf, path);
    buf.emitFrameStart();
    buf.emitWrite<quint16>(d3.id, 0, v2, v2);
    buf.emitWrite<quint16>(d1.id, 0, v2, v2);
    buf.emitWrite<quint16>(d2.id, 0, v2, v2);
    buf.emitFrameStart();
    auto frame = buf.cbegin();
    for (auto pkt = frame.cbegin(); pkt != frame.cend(); ++pkt)
      sink.analyze(pkt, sim::api2::trace::Direction::Forward);
    CHECK(sink.intervals().size() == 1);
    for (int it = 0; it <= 5; it++)
      CHECK(sink.contains(it));
  }
  SECTION("Insert contained in existing interval") {
    InfiniteBuffer buf;
    auto [bus, m1, m2, m3] = make(&buf);
    auto paths = QSharedPointer<sim::api2::Paths>::create();
    auto path = paths->add(0, bus->deviceID());
    TranslatingModifiedAddressSink<uint16_t> sink(paths, &*bus);
    auto guard = sim::api2::trace::PathGuard(&buf, path);
    buf.emitFrameStart();
    buf.emitWrite<quint16>(d3.id, 0, v2, v2);
    buf.emitWrite<quint16>(d3.id, 0, v1, v1);
    buf.emitFrameStart();
    auto frame = buf.cbegin();
    for (auto pkt = frame.cbegin(); pkt != frame.cend(); ++pkt)
      sink.analyze(pkt, sim::api2::trace::Direction::Forward);
    CHECK(sink.intervals().size() == 1);
    CHECK(sink.contains(4));
    CHECK(sink.contains(5));
  }
}
