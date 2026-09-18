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
#include "core/sim/memory/errors.hpp"
#include "core/sim/memory/ram/dense.hpp"
#include "core/sim/system.hpp"

namespace {
const auto rw = Operation{Operation::Type::Standard, Operation::Kind::data};

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
      .source = {.span = AddressSpan(0, 1)},
  });
  b1.mappings.push_back(Mapping{
      .target = m2->config().fullname,
      .source = {.span = AddressSpan(2, 3)},
  });
  b1.mappings.push_back(Mapping{
      .target = m3->config().fullname,
      .source = {.span = AddressSpan(4, 5)},
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

TEST_CASE("(new) SimpleBus permissions", "[scope:core][scope:core.sim][kind:int][arch:*]") {
  using Mapping = SimpleBus::Configuration::Mapping;
  const auto app = Operation{Operation::Type::Application, Operation::Kind::data};
  // One permission spanning the boundary between two devices, overlapping part of each.
  auto system = std::make_shared<System>();
  Target *lo = system->make_device<Dense>(Dense::Configuration{{.basename = "lo"}, 0, AddressSpan(0, 1000)});
  Target *hi = system->make_device<Dense>(Dense::Configuration{{.basename = "hi"}, 0, AddressSpan(0, 1499)});
  SimpleBus::Configuration cfg{{.basename = "bus"}, 0, AddressSpan(0, 3000)};
  cfg.mappings.push_back(Mapping{.target = "/lo", .source = {.span = AddressSpan(500, 1500)}});
  cfg.mappings.push_back(Mapping{.target = "/hi", .source = {.span = AddressSpan(1501, 3000)}});
  auto bus = system->make_device<SimpleBus>(cfg);
  system->initialize();
  const SimpleBus::Permission perms[] = {{AddressSpan(1000, 2000), Access::Read}};
  bus->apply_permissions(perms);

  u8 buf[2] = {0xAB, 0xCD};
  auto write_to_ro = [&](Address address, std::size_t length, Operation op) {
    try {
      bus->write(address, bits::span<const u8>{buf, length}, op);
    } catch (const Error &e) {
      return e.type() == Error::Type::WriteToRO;
    }
    return false;
  };

  SECTION("Only addresses inside the permission lose write access") {
    for (Address address : {500, 999, 2001, 3000}) CHECK_FALSE(write_to_ro(address, 1, rw));
    for (Address address : {1000, 1500, 1501, 2000}) CHECK(write_to_ro(address, 1, rw));
    // A write straddling the edge of the permission fails on its read-only byte.
    CHECK(write_to_ro(999, 2, rw));
  }
  SECTION("Split regions still reach the right device and offset") {
    REQUIRE_FALSE(write_to_ro(1000, 1, app));
    REQUIRE_FALSE(write_to_ro(2000, 1, app));
    u8 out[1] = {0};
    lo->read(500, {out}, rw);
    CHECK(out[0] == 0xAB);
    out[0] = 0;
    hi->read(499, {out}, rw);
    CHECK(out[0] == 0xAB);
    // Reads are not restricted.
    CHECK_NOTHROW(bus->read(1000, {out}, rw));
  }
  SECTION("Several unsorted permissions within one region") {
    const SimpleBus::Permission two[] = {{AddressSpan(800, 899), Access::Read}, {AddressSpan(600, 699), Access::Read}};
    bus->apply_permissions(two);
    for (Address address : {599, 700, 799, 900}) CHECK_FALSE(write_to_ro(address, 1, rw));
    for (Address address : {600, 699, 800, 899}) CHECK(write_to_ro(address, 1, rw));
  }
  SECTION("Overlapping permissions are rejected") {
    const SimpleBus::Permission overlapping[] = {{AddressSpan(600, 700), Access::Read},
                                                 {AddressSpan(700, 800), Access::Read}};
    CHECK_THROWS_AS(bus->apply_permissions(overlapping), std::invalid_argument);
  }
  SECTION("Permissions over unmapped addresses do not throw") {
    // [0, 399] is on the bus but has no device, [400, 599] only partly covers lo, and [4000, 5000] is off the bus.
    const SimpleBus::Permission unmapped[] = {{AddressSpan(0, 399), Access::Read},
                                              {AddressSpan(400, 599), Access::Read},
                                              {AddressSpan(4000, 5000), Access::Read}};
    REQUIRE_NOTHROW(bus->apply_permissions(unmapped));
    for (Address address : {500, 599}) CHECK(write_to_ro(address, 1, rw));
    for (Address address : {600, 1000, 3000}) CHECK_FALSE(write_to_ro(address, 1, rw));
  }
  SECTION("Applying no permissions restores the configured access") {
    bus->apply_permissions({});
    for (Address address : {1000, 1500, 1501, 2000}) CHECK_FALSE(write_to_ro(address, 1, rw));
  }
}

TEST_CASE("(new) Target::load fills a span", "[scope:core][scope:core.sim][kind:unit][arch:*]") {
  auto system = std::make_shared<System>();
  Target *mem = system->make_device<Dense>(Dense::Configuration{{.basename = "mem"}, 0xFF, AddressSpan(0, 99)});
  system->initialize();
  const u8 data[] = {1, 2, 3};

  SECTION("Bytes beyond the data are zero-filled") {
    mem->load(AddressSpan(10, 14), {data}, Access::ReadWriteExecute);
    u8 out[7] = {};
    mem->read(9, {out}, rw);
    CHECK(std::vector<u8>(out, out + 7) == std::vector<u8>{0xFF, 1, 2, 3, 0, 0, 0xFF});
  }
  SECTION("Data larger than its span is rejected") {
    CHECK_THROWS_AS(mem->load(AddressSpan(10, 11), {data}, Access::ReadWriteExecute), Error);
  }
}

TEST_CASE("(new) SimpleBus load", "[scope:core][scope:core.sim][kind:int][arch:*]") {
  using Mapping = SimpleBus::Configuration::Mapping;
  auto system = std::make_shared<System>();
  Target *lo = system->make_device<Dense>(Dense::Configuration{{.basename = "lo"}, 0, AddressSpan(0, 1000)});
  Target *hi = system->make_device<Dense>(Dense::Configuration{{.basename = "hi"}, 0, AddressSpan(0, 1499)});
  SimpleBus::Configuration cfg{{.basename = "bus"}, 0, AddressSpan(0, 3000)};
  cfg.mappings.push_back(Mapping{.target = "/lo", .source = {.span = AddressSpan(500, 1500)}});
  cfg.mappings.push_back(Mapping{.target = "/hi", .source = {.span = AddressSpan(1501, 3000)}});
  auto bus = system->make_device<SimpleBus>(cfg);
  system->initialize();

  const u8 image[] = {1, 2, 3, 4};
  auto write_to_ro = [&](Address address) {
    u8 byte = 0xAB;
    try {
      bus->write(address, bits::span<const u8>{&byte, 1}, rw);
    } catch (const Error &e) {
      return e.type() == Error::Type::WriteToRO;
    }
    return false;
  };
  auto peek = [&](Target *dev, Address address) {
    u8 out[1] = {0};
    dev->read(address, {out}, rw);
    return out[0];
  };

  SECTION("load fills copies data, zero-fills, and updates permissions") {
    bus->load(AddressSpan(600, 605), {image}, Access::Read);
    u8 out[6] = {};
    bus->read(600, {out}, rw);
    CHECK(std::vector<u8>(out, out + 6) == std::vector<u8>{1, 2, 3, 4, 0, 0});
    CHECK(write_to_ro(600));
    CHECK(write_to_ro(605));
    CHECK_FALSE(write_to_ro(599));
    CHECK_FALSE(write_to_ro(606));
  }
  SECTION("load can cross device boundaries") {
    bus->load(AddressSpan(1499, 1502), {image}, Access::ReadWriteExecute);
    CHECK(peek(lo, 999) == 1);
    CHECK(peek(lo, 1000) == 2);
    CHECK(peek(hi, 0) == 3);
    CHECK(peek(hi, 1) == 4);
  }
  SECTION("Most recent load wins") {
    bus->load(AddressSpan(600, 699), bits::span<const u8>{}, Access::Read);
    REQUIRE(write_to_ro(650));
    bus->load(AddressSpan(640, 660), bits::span<const u8>{}, Access::ReadWriteExecute);
    CHECK_FALSE(write_to_ro(650));
    // The part of the first load the second did not cover keeps its permission.
    CHECK(write_to_ro(639));
    CHECK(write_to_ro(661));
  }
  SECTION("Permissions are cleared on reset") {
    bus->load(AddressSpan(600, 699), bits::span<const u8>{}, Access::Read);
    REQUIRE(write_to_ro(650));
    bus->reset();
    CHECK_FALSE(write_to_ro(650));
  }
  SECTION("A load outside the bus is rejected") {
    CHECK_THROWS_AS(bus->load(AddressSpan(3000, 3005), {image}, Access::Read), Error);
  }
}
