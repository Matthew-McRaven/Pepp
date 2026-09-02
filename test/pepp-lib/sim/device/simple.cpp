/*
 * Copyright (c) 2024 J. Stanley Warford, Matthew McRaven
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

#include <catch.hpp>

#include "sim3/subsystems/bus/simple.hpp"
#include "sim3/subsystems/ram/dense.hpp"
#include "sim3/trace/buffers/infinite.hpp"
#include "sim3/trace/packet_utils.hpp"

namespace {
auto rw = sim::api2::memory::Operation{
    .type = sim::api2::memory::Operation::Type::Standard,
    .kind = sim::api2::memory::Operation::Kind::data,
};

auto d1 = sim::api2::device::Descriptor{.id = 1, .baseName = "d1", .fullName = "/bus0/d1"};
auto d2 = sim::api2::device::Descriptor{.id = 2, .baseName = "d2", .fullName = "/bus0/d2"};
auto d3 = sim::api2::device::Descriptor{.id = 3, .baseName = "d3", .fullName = "/bus0/d3"};
auto b1 = sim::api2::device::Descriptor{.id = 4, .baseName = "bus0", .fullName = "/bus0"};
auto b2 = sim::api2::device::Descriptor{.id = 5, .baseName = "bus1", .fullName = "/bus1"};
using Span = sim::api2::memory::AddressSpan<quint16>;
auto make = []() {
  auto m1 = QSharedPointer<sim::memory::Dense<quint16>>::create(d1, Span(0, 1));
  auto m2 = QSharedPointer<sim::memory::Dense<quint16>>::create(d2, Span(0, 1));
  auto m3 = QSharedPointer<sim::memory::Dense<quint16>>::create(d3, Span(0, 1));
  auto bus = QSharedPointer<sim::memory::SimpleBus<quint16>>::create(b1, Span(0, 5));
  CHECK(bus->deviceID() == b1.id);
  bus->pushFrontTarget(Span(0, 1), &*m1);
  bus->pushFrontTarget(Span(2, 3), &*m2);
  bus->pushFrontTarget(Span(4, 5), &*m3);
  return std::tuple{bus, m1, m2, m3};
};
} // namespace

TEST_CASE("Simple bus individual in-bounds access", "[scope:sim][kind:int][arch:*][!throws]") {
  auto [bus, m1, m2, m3] = make();
  sim::api2::memory::Target<quint16> *memArr[3] = {&*m1, &*m2, &*m3};
  quint8 buf[2];
  bits::span bufSpan = {buf};
  bits::memclr(bufSpan);

  // Can write to each individual memory and read on bus.
  for (int i = 0; i < 2; i++) {
    auto m = memArr[i];
    bits::memcpy_endian(bufSpan, bits::Order::BigEndian, quint16(0x0001));
    REQUIRE_NOTHROW(m->write(0, bufSpan, rw));
    bits::memclr(bufSpan);
    REQUIRE_NOTHROW(bus->read(0 + i * 2, bufSpan, rw));
    for (int j = 0; j < 1; j++)
      CHECK(buf[j] == j);
  }
}

TEST_CASE("Simple bus group in-bounds access", "[scope:sim][kind:int][arch:*][!throws]") {
  auto [bus, m1, m2, m3] = make();
  sim::api2::memory::Target<quint16> *memArr[3] = {&*m1, &*m2, &*m3};
  quint8 buf[6];
  bits::span bufSpan = {buf};
  for (int it = 0; it < 6; it++)
    buf[it] = it;
  REQUIRE_NOTHROW(bus->write(0, {buf}, rw));
  bits::memclr(bufSpan);

  // Can write to bus and read each individual memory.
  for (int i = 0; i < 2; i++) {
    auto m = memArr[i];
    REQUIRE_NOTHROW(m->read(0, bufSpan.first(2), rw));
    for (int j = 0; j < 1; j++)
      CHECK(buf[j] == i * 2 + j);
  }
}

TEST_CASE("Simple bus dump", "[scope:sim][kind:int][arch:*]") {
  SECTION("dense memory map") {
    auto [bus, m1, m2, m3] = make();
    sim::api2::memory::Target<quint16> *memArr[3] = {&*m1, &*m2, &*m3};
    quint8 buf[6];
    bits::span bufSpan = {buf};
    for (int it = 0; it < 6; it++)
      buf[it] = it;
    REQUIRE_NOTHROW(bus->write(0, {buf}, rw));
    bits::memclr(bufSpan);

    bus->dump({buf});

    for (int it = 0; it < 6; it++)
      CHECK(buf[it] == it);
  }
  SECTION("sparse memory map") {
    auto m1 = QSharedPointer<sim::memory::Dense<quint16>>::create(d1, Span(0, 1));
    auto m2 = QSharedPointer<sim::memory::Dense<quint16>>::create(d2, Span(0, 1));
    auto m3 = QSharedPointer<sim::memory::Dense<quint16>>::create(d3, Span(0, 1));
    auto bus = QSharedPointer<sim::memory::SimpleBus<quint16>>::create(b1, Span(0, 9));
    bus->pushFrontTarget(Span(0, 1), &*m1);
    bus->pushFrontTarget(Span(4, 5), &*m2);
    bus->pushFrontTarget(Span(8, 9), &*m3);
    quint8 buf[10], out[10];
    bits::span bufSpan = {buf};
    bits::span outSpan = {out};
    bits::memclr(bufSpan);
    bits::memclr(outSpan);
    bits::memcpy_endian(bufSpan.subspan(0, 2), bits::Order::BigEndian, quint16(0x0001));
    m1->write(0, bufSpan.subspan(0, 2), rw);
    bits::memcpy_endian(bufSpan.subspan(4, 2), bits::Order::BigEndian, quint16(0x0405));
    m2->write(0, bufSpan.subspan(4, 2), rw);
    bits::memcpy_endian(bufSpan.subspan(8, 2), bits::Order::BigEndian, quint16(0x0809));
    m3->write(0, bufSpan.subspan(8, 2), rw);

    bus->dump({out});
    for (int it = 0; it < sizeof(out); it++)
      CHECK(buf[it] == out[it]);
  }
}

TEST_CASE("Simple bus path tracking", "[scope:sim][kind:int][arch:*]") {
  auto mem = QSharedPointer<sim::memory::Dense<quint16>>::create(d1, Span(0, 1));
  auto bus0 = QSharedPointer<sim::memory::SimpleBus<quint16>>::create(b1, Span(0, 5));
  bus0->pushFrontTarget({0, 1}, &*mem);
  auto bus1 = QSharedPointer<sim::memory::SimpleBus<quint16>>::create(b2, Span(0, 5));
  bus1->pushFrontTarget({2, 3}, &*mem);

  auto paths = QSharedPointer<sim::api2::Paths>::create();
  auto path0 = paths->add(0, bus0->deviceID());
  auto path1 = paths->add(0, bus1->deviceID());
  auto tb = QSharedPointer<sim::trace2::InfiniteBuffer>::create();
  for (auto &bus : {bus0, bus1}) {
    bus->setPathManager(paths);
    bus->setBuffer(&*tb);
    bus->trace(true);
  }
  mem->setBuffer(&*tb);
  mem->trace(true);

  quint8 buf[2];
  for (int it = 0; it < 2; it++)
    buf[it] = it + 1;
  tb->emitFrameStart();
  bus0->write(0, {buf}, rw);
  bus1->write(2, {buf}, rw);
  tb->emitFrameStart();
  auto frames = tb->cbegin();
  auto packets = frames.cbegin();
  REQUIRE(packets != frames.cend());
  CHECK(sim::trace2::get_path(*packets) == path0);
  CHECK(sim::trace2::get_address<quint16>(*packets) == std::make_optional<quint16>(0));
  ++packets;
  REQUIRE(packets != frames.cend());
  CHECK(sim::trace2::get_path(*packets) == path1);
  CHECK(sim::trace2::get_address<quint16>(*packets) == std::make_optional<quint16>(0));
}
