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

#include "core/sim/memory/ram/sparse.hpp"
#include <catch.hpp>
#include "./compare.hpp"
#include "core/sim/memory/errors.hpp"

namespace {
auto desc = Device::Configuration{.basename = "dev", .fullname = "/dev"};
auto op = Operation{
    .type = Operation::Type::Standard,
    .kind = Operation::Kind::data,
};

} // namespace

TEST_CASE("Sparse storage in-bounds access", "[scope:core][scope:core.sim][kind:int][arch:*]") {
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
  auto span = AddressSpan(offset, 0x1000);

  // Initialize a memory block to a fixed value
  Sparse dev(desc, Device::ID{}, span, 0xFE);

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

  // Cross a page boundary if len>1
  REQUIRE_NOTHROW(dev.write(0xFE, {truth, length}, op));
  REQUIRE_NOTHROW(dev.read(0xFE, {tmp, length}, op));
  compare(truth, tmp, length);
}

TEST_CASE("Sparse storage out-of-bounds access", "[scope:core][scope:core.sim][kind:int][arch:*][!throws]") {
  auto span = AddressSpan(0xFE, 0xFE);

  // Initialize a memory block to a fixed value
  Sparse dev(desc, Device::ID{}, span, 0xFE);

  // Create an 8-byte temporary buffer.
  u64 reg = 0;
  u8 *tmp = (u8 *)&reg;
  // In-bound read does not throw, and retrives default value.
  REQUIRE_NOTHROW(dev.read(0xFE, {tmp, 1}, op));

  // Initialize tmp to be different than dev default value.
  // Neither OOB read should update temp.
  *tmp = 0xCA;
  REQUIRE_THROWS_AS(dev.read(0xFD, {tmp, 1}, op), Error);
  CHECK(*tmp == 0xCA);
  REQUIRE_THROWS_AS(dev.read(0xFF, {tmp, 1}, op), Error);
  CHECK(*tmp == 0xCA);
  // Cross a page boundary
  REQUIRE_THROWS_AS(dev.read(0x100, {tmp, 1}, op), Error);
  CHECK(*tmp == 0xCA);

  // Neither write will stick, so tmp is meaningless
  *tmp = 0xfe;
  REQUIRE_THROWS_AS(dev.write(0xFD, {tmp, 1}, op), Error);
  REQUIRE_THROWS_AS(dev.write(0xFF, {tmp, 1}, op), Error);
  // Cross a page boundary
  REQUIRE_THROWS_AS(dev.write(0x100, {tmp, 1}, op), Error);
}
