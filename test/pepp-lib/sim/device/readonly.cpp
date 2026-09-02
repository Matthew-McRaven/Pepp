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
#include "sim3/subsystems/ram/readonly.hpp"

namespace {
namespace api2 = sim::api2;
auto desc_rw = api2::device::Descriptor{.id = 0, .compatible = nullptr, .baseName = "dev_rw", .fullName = "/dev_rw"};
auto op_std = api2::memory::Operation{
    .type = api2::memory::Operation::Type::Standard,
    .kind = api2::memory::Operation::Kind::data,
};

auto op_app = api2::memory::Operation{
    .type = api2::memory::Operation::Type::Application,
    .kind = api2::memory::Operation::Kind::data,
};

void compare(const quint8 *lhs, const quint8 *rhs, quint8 length) {
  if (lhs == nullptr || rhs == nullptr)
    return;
  for (int it = 0; it < length; it++)
    CHECK(lhs[it] == rhs[it]);
};

void compare_ne(const quint8 *lhs, const quint8 *rhs, quint8 length) {
  if (lhs == nullptr || rhs == nullptr)
    return;
  for (int it = 0; it < length; it++)
    CHECK(lhs[it] != rhs[it]);
};
} // namespace

TEST_CASE("Read-only storage in-bounds access", "[scope:sim][kind:int][arch:*][!throws]") {

  auto span = api2::memory::AddressSpan<quint8>(0x10, 255);

  // Initialize a memory block to a fixed value
  sim::memory::Dense<quint8> dev_rw(desc_rw, span, 0xFE);
  sim::memory::ReadOnly<quint8> dev_ro(false);
  dev_ro.setTarget(&dev_rw, nullptr);
  CHECK(dev_ro.deviceID() == desc_rw.id);
  CHECK(dev_rw.deviceID() == desc_rw.id);

  // Create an 8-byte temporary buffer.
  quint8 length = 4;
  quint64 reg = 0;
  quint8 *tmp = (quint8 *)&reg;
  // In-bound read does not throw, and retrives default value.
  REQUIRE_NOTHROW(dev_ro.read(0x10, {tmp, length}, op_std));
  auto vec = QList<quint8>(length, 0xFE);
  compare(vec.constData(), tmp, length);

  // Read after write does not observe changes,
  // but does not error when hardfail is false
  reg = 0xFEEDBEEF;
  const quint8 truth[8] = {0, 1, 2, 3, 4, 5, 6, 7};
  REQUIRE_NOTHROW(dev_ro.write(0x10, {truth, length}, op_std));
  REQUIRE_NOTHROW(dev_ro.read(0x10, {tmp, length}, op_std));
  compare_ne(truth, tmp, length);

  // RO storage can be overriden with "application" op
  REQUIRE_NOTHROW(dev_ro.write(0x10, {truth, length}, op_app));
  REQUIRE_NOTHROW(dev_ro.read(0x10, {tmp, length}, op_std));
  compare(truth, tmp, length);

  dev_rw.clear(0xFE);
  // Read after write does not observe changes,
  // but does not error when hardfail is false
  sim::memory::ReadOnly<quint8> dev_ro_throws(true);
  dev_ro_throws.setTarget(&dev_rw, nullptr);

  // Read after write does not observe changes,
  // but does not error when hardfail is false
  reg = 0xFEEDBEEF;
  REQUIRE_THROWS_AS(dev_ro_throws.write(0x10, {truth, length}, op_std), api2::memory::Error);
  compare_ne(truth, tmp, length);

  // hardfail RO storage can be overriden with "application" op
  REQUIRE_NOTHROW(dev_ro_throws.write(0x10, {truth, length}, op_app));
  REQUIRE_NOTHROW(dev_ro_throws.read(0x10, {tmp, length}, op_std));
  compare(truth, tmp, length);
}

TEST_CASE("Read-only storage out-of-bounds access", "[scope:sim][kind:int][arch:*][!throws]") {
  auto span = api2::memory::AddressSpan<quint8>(0x10, 0x10);

  // Initialize a memory block to a fixed value
  sim::memory::Dense<quint8> dev_rw(desc_rw, span, 0xFE);
  sim::memory::ReadOnly<quint8> dev_ro(true);
  dev_ro.setTarget(&dev_rw, nullptr);

  // Create an 8-byte temporary buffer.
  quint64 reg = 0;
  quint8 *tmp = (quint8 *)&reg;

  // Neither write will stick, so tmp is meaningless
  *tmp = 0xfe;
  REQUIRE_THROWS_AS(dev_ro.write(0x9, {tmp, 1}, op_std), api2::memory::Error);
  REQUIRE_THROWS_AS(dev_ro.write(0x11, {tmp, 1}, op_std), api2::memory::Error);
}
