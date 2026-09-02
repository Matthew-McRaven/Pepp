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

#include "core/sim/memory/ram/dense.hpp"
#include <catch.hpp>
#include <vector>
#include "./compare.hpp"
#include "core/sim/memory/errors.hpp"

namespace {
auto base_desc = Device::Configuration{.basename = "dev", .fullname = "/dev"};
auto op = Operation{Operation::Type::Standard, Operation::Kind::data};
Dense make_dense(AddressSpan span, u8 fill = 0xFE) {
  auto cfg = Dense::Configuration{Device::Configuration{base_desc}};
  cfg.span = span, cfg.fill = fill, cfg.id = {};
  return Dense(cfg);
}
} // namespace

TEST_CASE("(new) Dense storage in-bounds access", "[scope:core][scope:core.sim][kind:int][arch:*]") {
  auto [length, offset] = GENERATE(table<u8, u8>({
      {1, 0},
      {2, 0},
      {4, 0},
      {8, 0},
      {1, 8},
      {2, 8},
      {4, 8},
      {8, 8},
  }));
  // Initialize a memory block to a fixed value
  auto dev = make_dense(AddressSpan(offset, 255), 0xFE);

  // Create an 8-byte temporary buffer.
  u64 reg = 0;
  u8 *tmp = (u8 *)&reg;
  // In-bound read does not throw, and retrives default value.
  REQUIRE_NOTHROW(dev.read(0x10, {tmp, length}, op));
  auto vec = std::vector<u8>(length, 0xFE);
  compare(vec.data(), tmp, length);

  // Read after write observes changes.
  const u8 truth[8] = {0, 1, 2, 3, 4, 5, 6, 7};
  REQUIRE_NOTHROW(dev.write(0x10, {truth, length}, op));
  REQUIRE_NOTHROW(dev.read(0x10, {tmp, length}, op));
  compare(truth, tmp, length);
  // Check that data ends up in correct location in backing store.
  // i.e., the read API didn't do some awful bitmath it wasn't supposed to,
  // or that the API didn't byteswap.
  compare(dev.data().data() + 0x10 - offset, truth, length);
}

TEST_CASE("(new) Dense storage out-of-bounds access", "[scope:core][scope:core.sim][kind:int][arch:*][!throws]") {
  auto span = AddressSpan(0x10, 0x10);

  auto cfg = Dense::Configuration{Device::Configuration{base_desc}};
  cfg.span = span, cfg.fill = 0xFE, cfg.id = {};
  // Initialize a memory block to a fixed value
  Dense dev(cfg);

  // Create an 8-byte temporary buffer.
  u64 reg = 0;
  u8 *tmp = (u8 *)&reg;
  // In-bound read does not throw, and retrives default value.
  REQUIRE_NOTHROW(dev.read(0x10, {tmp, 1}, op));

  // Initialize tmp to be different than dev default value.
  // Neither OOB read should update temp.
  *tmp = 0xCA;
  REQUIRE_THROWS_AS(dev.read(0x9, {tmp, 1}, op), Error);
  CHECK(*tmp == 0xCA);
  REQUIRE_THROWS_AS(dev.read(0x11, {tmp, 1}, op), Error);
  CHECK(*tmp == 0xCA);

  // Neither write will stick, so tmp is meaningless
  *tmp = 0xfe;
  REQUIRE_THROWS_AS(dev.write(0x9, {tmp, 1}, op), Error);
  REQUIRE_THROWS_AS(dev.write(0x11, {tmp, 1}, op), Error);
}

TEST_CASE("(new) Dense change tracking", "[scope:core][scope:core.sim][kind:unit][arch:*][!throws]") {
  // A non-zero lower bound, so that a device offset can never be mistaken for an address.
  static constexpr Address base = 0x10, last = 0xFF;
  auto span = AddressSpan(base, last);

  SECTION("device's default state is no changes") {
    auto dev = make_dense(span);
    CHECK(changes_of(dev) == Changes{});
    dev.clear_changes(); // No-op if empty
    CHECK(changes_of(dev) == Changes{});
  }

  SECTION("reads do not affect change tracking") {
    auto dev = make_dense(span);
    u64 reg = 0;
    dev.read(base, {(u8 *)&reg, 8}, op);
    CHECK(changes_of(dev) == Changes{});
  }

  SECTION("write dirties correct bits") {
    // 1/2/4/8 cover specialized switch branches, 3/5/16 take the memcpy route.
    auto length = GENERATE(as<std::size_t>{}, 1, 2, 3, 4, 5, 8, 16);
    auto dev = make_dense(span);
    poke(dev, 0x20, length, op);
    CHECK(changes_of(dev) == Changes{{0x20, Address(0x20 + length - 1)}});
  }

  SECTION("write_increment dirties correct bits") {
    auto dev = make_dense(span);
    dev.write_increment<u16, false>(0x20, 0xBEEF, op);
    CHECK(changes_of(dev) == Changes{{0x20, 0x21}});
  }

  SECTION("handles spans at the extremes") {
    // The run at `last` is terminated by running out of device rather than by a clean byte.
    auto dev = make_dense(span);
    poke(dev, base, 1, op);
    poke(dev, last, 1, op);
    CHECK(changes_of(dev) == Changes{{base, base}, {last, last}});
  }

  SECTION("collect_changes does not modify Target") {
    auto dev = make_dense(span);
    poke(dev, 0x20, 1, op);
    CHECK(changes_of(dev) == Changes{{0x20, 0x20}});
    CHECK(changes_of(dev) == Changes{{0x20, 0x20}});
  }
  SECTION("collect_changes does not clear set") {
    auto dev = make_dense(span);
    poke(dev, 0x20, 1, op);
    // Changes are added to whatever the caller already had, so one set can span many devices.
    pepp::core::IntervalSet<Address> set;
    set.insert(0x80, 0x81);
    dev.collect_changes(set);
    CHECK(set.intervals() == Changes{{0x20, 0x20}, {0x80, 0x81}});
  }

  SECTION("An out-of-bounds write leaves change tracking untouched") {
    auto dev = make_dense(span);
    const u8 v = 0xCA;
    CHECK_THROWS_AS(dev.write(base - 1, {&v, 1}, op), Error);
    CHECK_THROWS_AS(dev.write(last, {&v, 2}, op), Error);
    CHECK(changes_of(dev) == Changes{});
  }

  SECTION("clear_changes does not affect data") {
    auto dev = make_dense(span);
    poke(dev, 0x20, 4, op);
    dev.clear_changes();
    CHECK(changes_of(dev) == Changes{});
    CHECK(dev.read<u32>(0x20, op).second != 0);
    // Only writes made after the clear are reported.
    poke(dev, 0x60, 2, op);
    CHECK(changes_of(dev) == Changes{{0x60, 0x61}});
  }

  SECTION("Empty spans") {
    auto empty = make_dense(AddressSpan{});
    CHECK(changes_of(empty) == Changes{});
  }

  SECTION("clear() discards accumulated changes") {
    auto dev = make_dense(span);
    poke(dev, 0x20, 4, op);
    dev.clear(0x00);
    CHECK(changes_of(dev) == Changes{});
  }
}
