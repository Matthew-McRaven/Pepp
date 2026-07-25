/*
 * Copyright (c) 2024-2026 J. Stanley Warford, Matthew McRaven
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

#include "core/sim/memory/bus/simplebus.hpp"
#include "core/sim/memory/ram/dense.hpp"
#include "core/sim/system.hpp"

namespace {
auto rw = Operation{
    .type = Operation::Type::Standard,
    .kind = Operation::Kind::data,
};

auto d1 = Dense::Configuration{{.basename = "d1", .fullname = "/bus0/d1"}, 0, AddressSpan(0, 1)};
auto d2 = Dense::Configuration{{.basename = "d2", .fullname = "/bus0/d2"}, 0, AddressSpan(0, 1)};
auto d3 = Dense::Configuration{{.basename = "d3", .fullname = "/bus0/d3"}, 0, AddressSpan(0, 1)};
auto base_b1 = Device::Configuration{.basename = "bus0", .fullname = "/bus0"};

auto make = []() {
  using Mapping = SimpleBus::Configuration::Mapping;
  auto system = std::make_shared<System>();
  auto m1 = system->make_device<Dense>(d1);
  auto m2 = system->make_device<Dense>(d2);
  auto m3 = system->make_device<Dense>(d3);
  SimpleBus::Configuration b1{{base_b1}, 0, AddressSpan(0, 5)};
  b1.mappings.push_back(Mapping{
      .target = m1->config().fullname,
      .source_span = AddressSpan(0, 1),
  });
  b1.mappings.push_back(Mapping{
      .target = m2->config().fullname,
      .source_span = AddressSpan(2, 3),
  });
  b1.mappings.push_back(Mapping{
      .target = m3->config().fullname,
      .source_span = AddressSpan(4, 5),
  });

  auto bus = system->make_device<SimpleBus>(b1);
  system->initialize();
  return std::tuple{system, bus, m1, m2, m3};
};
} // namespace

TEST_CASE("(new) SimpleBus storage in-bounds access", "[scope:core][scope:core.sim][kind:int][arch:*]") {
  auto [sys, bus, m1, m2, m3] = make();
  Target *memArr[3] = {&*m1, &*m2, &*m3};
  u8 buf[2];
  bits::span bufSpan = {buf};
  bits::memclr(bufSpan);

  // Can write to each individual memory and read on bus.
  for (int i = 0; i < 3; i++) {
    auto m = memArr[i];
    bits::memcpy_endian(bufSpan, bits::Order::BigEndian, u16(0x0001));
    REQUIRE_NOTHROW(m->write(0, bufSpan, rw));
    bits::memclr(bufSpan);
    REQUIRE_NOTHROW(bus->read(0 + i * 2, bufSpan, rw));
    CHECK(buf[0] == 0);
    CHECK(buf[1] == 1);
  }
}

TEST_CASE("(new) SimpleBus group in-bounds access", "[scope:core][scope:core.sim][kind:int][arch:*]") {
  auto [sys, bus, m1, m2, m3] = make();
  Target *memArr[3] = {&*m1, &*m2, &*m3};
  u8 buf[6];
  bits::span bufSpan = {buf};
  for (int it = 0; it < 6; it++) buf[it] = it;
  REQUIRE_NOTHROW(bus->write(0, {buf}, rw));
  bits::memclr(bufSpan);

  // Can write to bus and read each individual memory.
  for (int i = 0; i < 3; i++) {
    auto m = memArr[i];
    REQUIRE_NOTHROW(m->read(0, bufSpan.first(2), rw));
    CHECK(buf[0] == i * 2 + 0);
    CHECK(buf[1] == i * 2 + 1);
  }
}
