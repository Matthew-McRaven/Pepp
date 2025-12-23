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

#include "sim3/subsystems/ram/dense.hpp"
namespace {
namespace api2 = sim::api2;
auto desc = api2::device::Descriptor{.id = 0, .compatible = nullptr, .baseName = "dev", .fullName = "/dev"};
auto op = api2::memory::Operation{
    .type = api2::memory::Operation::Type::Standard,
    .kind = api2::memory::Operation::Kind::data,
};

void compare(const quint8 *lhs, const quint8 *rhs, quint8 length) {
  if (lhs == nullptr || rhs == nullptr)
    return;
  for (int it = 0; it < length; it++)
    CHECK(lhs[it] == rhs[it]);
};
} // namespace

TEST_CASE("Dense storage in-bounds access", "[scope:sim][kind:int][arch:*]") {
  auto [length, offset] = GENERATE(table<quint8, quint8>({
      {1, 0},
      {2, 0},
      {4, 0},
      {8, 0},
      {1, 8},
      {2, 8},
      {4, 8},
      {8, 8},
  }));
  auto span = api2::memory::AddressSpan<quint8>(offset, 255);

  // Initialize a memory block to a fixed value
  sim::memory::Dense<quint8> dev(desc, span, 0xFE);
  CHECK(dev.deviceID() == desc.id);

  // Create an 8-byte temporary buffer.
  quint64 reg = 0;
  quint8 *tmp = (quint8 *)&reg;
  // In-bound read does not throw, and retrives default value.
  REQUIRE_NOTHROW(dev.read(0x10, {tmp, length}, op));
  auto vec = QList<quint8>(length, 0xFE);
  compare(vec.constData(), tmp, length);

  // Read after write observes changes.
  const quint8 truth[8] = {0, 1, 2, 3, 4, 5, 6, 7};
  REQUIRE_NOTHROW(dev.write(0x10, {truth, length}, op));
  REQUIRE_NOTHROW(dev.read(0x10, {tmp, length}, op));
  compare(truth, tmp, length);
  // Check that data ends up in correct location in backing store.
  // i.e., the read API didn't do some awful bitmath it wasn't supposed to,
  // or that the API didn't byteswap.
  compare(dev.constData() + 0x10 - offset, truth, length);
}

TEST_CASE("Dense storage out-of-bounds access", "[scope:sim][kind:int][arch:*][!throws]") {
  auto span = api2::memory::AddressSpan<quint8>(0x10, 0x10);

  // Initialize a memory block to a fixed value
  sim::memory::Dense<quint8> dev(desc, span, 0xFE);

  // Create an 8-byte temporary buffer.
  quint64 reg = 0;
  quint8 *tmp = (quint8 *)&reg;
  // In-bound read does not throw, and retrives default value.
  REQUIRE_NOTHROW(dev.read(0x10, {tmp, 1}, op));

  // Initialize tmp to be different than dev default value.
  // Neither OOB read should update temp.
  *tmp = 0xCA;
  REQUIRE_THROWS_AS(dev.read(0x9, {tmp, 1}, op), api2::memory::Error);
  CHECK(*tmp == 0xCA);
  REQUIRE_THROWS_AS(dev.read(0x11, {tmp, 1}, op), api2::memory::Error);
  CHECK(*tmp == 0xCA);

  // Neither write will stick, so tmp is meaningless
  *tmp = 0xfe;
  REQUIRE_THROWS_AS(dev.write(0x9, {tmp, 1}, op), api2::memory::Error);
  REQUIRE_THROWS_AS(dev.write(0x11, {tmp, 1}, op), api2::memory::Error);
}
