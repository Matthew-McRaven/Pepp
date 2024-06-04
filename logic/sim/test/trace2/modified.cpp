#include "sim/trace2/modified.hpp"
#include <catch.hpp>
#include "sim/trace2/buffers.hpp"

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
using IS = IntervalSet<uint16_t>;
TEST_CASE("IntervalSet", "[scope:sim][kind:unit][arch:*]") {
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

TEST_CASE("ModifiedAddressSink", "[scope:sim][kind:unit][arch:*]") {
  using namespace sim::api2::packet;
  quint8 v1[] = {0};
  quint8 v2[] = {0, 0};
  SECTION("Append-only, no merge") {
    InfiniteBuffer buf;
    buf.trace(0, true);
    ModifiedAddressSink<uint16_t> sink;
    emitFrameStart(&buf);
    emitWrite<quint16>(&buf, 0, 0u, v1, v1);
    emitWrite<quint16>(&buf, 0, 2u, v1, v1);
    emitWrite<quint16>(&buf, 0, 4u, v1, v1);
    emitFrameStart(&buf);
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
    emitFrameStart(&buf);
    emitWrite<quint16>(&buf, 0, 0u, v2, v2);
    emitWrite<quint16>(&buf, 0, 2u, v2, v2);
    emitWrite<quint16>(&buf, 0, 4u, v2, v2);
    emitFrameStart(&buf);
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
    emitFrameStart(&buf);
    emitWrite<quint16>(&buf, 0, 4u, v1, v1);
    emitWrite<quint16>(&buf, 0, 2u, v1, v1);
    emitWrite<quint16>(&buf, 0, 0, v1, v1);
    emitFrameStart(&buf);
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
    emitFrameStart(&buf);
    emitWrite<quint16>(&buf, 0, 4, v2, v2);
    emitWrite<quint16>(&buf, 0, 2, v2, v2);
    emitWrite<quint16>(&buf, 0, 0, v2, v2);
    emitFrameStart(&buf);
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
    emitFrameStart(&buf);
    emitWrite<quint16>(&buf, 0, 0, v2, v2);
    emitWrite<quint16>(&buf, 0, 4, v2, v2);
    emitWrite<quint16>(&buf, 0, 2, v2, v2);
    emitFrameStart(&buf);
    auto frame = buf.cbegin();
    for (auto pkt = frame.cbegin(); pkt != frame.cend(); ++pkt)
      sink.analyze(pkt, sim::api2::trace::Direction::Forward);
    CHECK(sink.intervals().size() == 1);
    for (int it = 0; it <= 5; it++)
      CHECK(sink.contains(it));
    IS set;
  }
  SECTION("Insert contained in existing interval") {
    InfiniteBuffer buf;
    buf.trace(0, true);
    ModifiedAddressSink<uint16_t> sink;
    emitFrameStart(&buf);
    emitWrite<quint16>(&buf, 0, 0, v2, v2);
    emitWrite<quint16>(&buf, 0, 0, v1, v1);
    emitFrameStart(&buf);
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
    emitFrameStart(&buf);
    emitWrite<quint16>(&buf, 0, 0, v1, v1);
    emitWrite<quint16>(&buf, 0, 2, v1, v1);
    emitWrite<quint16>(&buf, 0, 4, v1, v1);
    emitWrite<quint16>(&buf, 0, 6, v1, v1);
    emitFrameStart(&buf);
    auto frame = buf.cbegin();
    for (auto pkt = frame.cbegin(); pkt != frame.cend(); ++pkt)
      sink.analyze(pkt, sim::api2::trace::Direction::Forward);
    CHECK(sink.intervals().size() == 4);
    for (int it = 0; it <= 7; it++)
      CHECK(sink.contains(it) == (it % 2 == 0));
    buf.clear();
    quint8 v5[] = {1, 2, 3, 4, 5};
    emitFrameStart(&buf);
    emitWrite<quint16>(&buf, 0, 1, v5, v5);
    emitFrameStart(&buf);
    frame = buf.cbegin();
    for (auto pkt = frame.cbegin(); pkt != frame.cend(); ++pkt)
      sink.analyze(pkt, sim::api2::trace::Direction::Forward);
    CHECK(sink.intervals().size() == 1);
    for (int it = 0; it <= 6; it++)
      CHECK(sink.contains(it));
    CHECK_FALSE(sink.contains(7));
  }
  SECTION("Wrap-around") {
    // TODO, write that wraps around from 0xFFFF to 0x0000
  }
}
