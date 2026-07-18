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

#include "core/sim/memory/io/mm.hpp"
#include <catch.hpp>
#include "core/sim/memory/errors.hpp"

namespace {
auto base_desc = Device::Configuration{.basename = "dev", .fullname = "/dev"};
auto op = Operation{
    .type = Operation::Type::Standard,
    .kind = Operation::Kind::data,
};

} // namespace

TEST_CASE("(new) MemoryMappedReg  storage in-bounds access", "[scope:core][scope:core.sim][kind:int][arch:*]") {
  auto [offset] = GENERATE(table<u8>({
      {0},
      {8},
      {16},

  }));
  auto span = AddressSpan(offset, offset);
  const u32 length = pepp::core::size_inclusive(span);
  auto cfg = MemoryMappedRegister::Configuration{Device::Configuration{base_desc}};
  cfg.span = span, cfg.fill = 0xFE, cfg.id = {};

  // Create an 8-byte temporary buffer.
  u64 reg = 0;
  u8 *tmp = (u8 *)&reg;

  SECTION("read from read-only") {
    const u8 buffered = 19;
    auto local = cfg;
    local.direction = MemoryMappedRegister::IODirection::Input;
    MemoryMappedRegister dev(local);
    auto &i = dev.input();
    i.push(buffered);

    // In-bound read does not throw, and retrives default value.
    REQUIRE_NOTHROW(dev.read(offset, {tmp, length}, op));
    CHECK((i16)*tmp == (i16)buffered);
  }

  // writeonly will not consume from input queue.
  SECTION("read from write-only") {
    auto local = cfg;
    local.direction = MemoryMappedRegister::IODirection::Output;
    MemoryMappedRegister dev(local);
    auto &i = dev.input();
    auto &o = dev.output();
    auto ob = o.end();
    i.push(19);

    // In-bound read does not throw, and retrives default value.
    REQUIRE_NOTHROW(dev.read(offset, {tmp, length}, op));
    CHECK((i16)*tmp == (i16)cfg.fill);
    // output queue should not have been modified
    CHECK(o.end() == ob);
  }

  // write to readonly is ignored
  SECTION("write to readonly-only") {
    auto local = cfg;
    local.direction = MemoryMappedRegister::IODirection::Input;
    MemoryMappedRegister dev(local);
    auto &o = dev.output();
    auto ob = o.end();
    *tmp = 0xCA;

    // In-bound write does not throw.
    REQUIRE_NOTHROW(dev.write(offset, {tmp, length}, op));
    // output queue should not have been modified
    CHECK(o.end() == ob);
    CHECK(o.latest_or(0) == 0);
    CHECK(ob.at_end());
  }

  // write to writeonly updates output queue.
  SECTION("write to write-only") {
    auto local = cfg;
    local.direction = MemoryMappedRegister::IODirection::Output;
    MemoryMappedRegister dev(local);
    auto &o = dev.output();
    auto ob = o.end();
    *tmp = 0xCA;

    // In-bound write does not throw.
    REQUIRE_NOTHROW(dev.write(offset, {tmp, length}, op));

    // output queue should not have been modified
    CHECK(o.end() != ob);
    CHECK(o.latest_or(0) == 0xCA);
    CHECK(!ob.at_end());
    CHECK(*ob == 0xCA);
  }

  SECTION("Return default value when fail_policy == YieldDefaultValue") {
    auto span = AddressSpan(0x17, 0x17);
    auto cfg = MemoryMappedRegister::Configuration{Device::Configuration{base_desc}};
    cfg.span = span, cfg.fill = 0xFE, cfg.id = {};
    cfg.direction = MemoryMappedRegister::IODirection::Input;
    cfg.fail_policy = FailPolicy::YieldDefaultValue;
    MemoryMappedRegister dev(cfg);
    *tmp = 0;
    REQUIRE_NOTHROW(dev.read(0x17, {tmp, 1}, op));
    CHECK(*tmp == 0xFE);
  }
}

TEST_CASE("(new) MemoryMappedReg storage out-of-bounds access",
          "[scope:core][scope:core.sim][kind:int][arch:*][!throws]") {
  u64 reg = 0;
  u8 *tmp = (u8 *)&reg;
  SECTION("MMReg must be single byte") {
    auto span = AddressSpan(255, 400);
    auto cfg = MemoryMappedRegister::Configuration{Device::Configuration{base_desc}};
    cfg.span = span, cfg.fill = 0xFE, cfg.id = {};
    REQUIRE_THROWS_AS([&cfg]() { MemoryMappedRegister dev(cfg); }(), std::logic_error);
  }
  SECTION("OOB read + write to inputonly") {
    auto span = AddressSpan(0x17, 0x17);
    auto cfg = MemoryMappedRegister::Configuration{Device::Configuration{base_desc}};
    cfg.span = span, cfg.fill = 0xFE, cfg.id = {};
    cfg.direction = MemoryMappedRegister::IODirection::Input;
    MemoryMappedRegister dev(cfg);
    REQUIRE_THROWS_AS(dev.read(0x16, {tmp, 1}, op), Error);
    REQUIRE_THROWS_AS(dev.read(0x18, {tmp, 1}, op), Error);
    REQUIRE_THROWS_AS(dev.write(0x16, {tmp, 1}, op), Error);
    REQUIRE_THROWS_AS(dev.write(0x18, {tmp, 1}, op), Error);
  }
  SECTION("OOB read + write to writeonly") {
    auto span = AddressSpan(0x17, 0x17);
    auto cfg = MemoryMappedRegister::Configuration{Device::Configuration{base_desc}};
    cfg.span = span, cfg.fill = 0xFE, cfg.id = {};
    cfg.direction = MemoryMappedRegister::IODirection::Output;
    MemoryMappedRegister dev(cfg);
    REQUIRE_THROWS_AS(dev.read(0x16, {tmp, 1}, op), Error);
    REQUIRE_THROWS_AS(dev.read(0x18, {tmp, 1}, op), Error);
    REQUIRE_THROWS_AS(dev.write(0x16, {tmp, 1}, op), Error);
    REQUIRE_THROWS_AS(dev.write(0x18, {tmp, 1}, op), Error);
  }
  SECTION("Throw when out of MMI") {
    auto span = AddressSpan(0x17, 0x17);
    auto cfg = MemoryMappedRegister::Configuration{Device::Configuration{base_desc}};
    cfg.span = span, cfg.fill = 0xFE, cfg.id = {};
    cfg.direction = MemoryMappedRegister::IODirection::Input;
    MemoryMappedRegister dev(cfg);
    REQUIRE_THROWS_AS(dev.read(0x17, {tmp, 1}, op), Error);
  }
}
