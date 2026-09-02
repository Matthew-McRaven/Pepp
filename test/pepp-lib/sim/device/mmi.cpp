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

#include "sim3/subsystems/ram/broadcast/mmi.hpp"

namespace {
auto desc = sim::api2::device::Descriptor{.id = 1, .baseName = "cin", .fullName = "/cin"};

auto rw = sim::api2::memory::Operation{
    .type = sim::api2::memory::Operation::Type::Standard,
    .kind = sim::api2::memory::Operation::Kind::data,
};
auto app = sim::api2::memory::Operation{
    .type = sim::api2::memory::Operation::Type::Application,
    .kind = sim::api2::memory::Operation::Kind::data,
};
auto span = sim::api2::memory::AddressSpan<quint16>(0, 0);
} // namespace

TEST_CASE("Memory-mapped input read", "[scope:sim][kind:int][arch:*]") {
  auto in = QSharedPointer<sim::memory::Input<quint16>>::create(desc, span, 0);
  CHECK(in->deviceID() == desc.id);
  auto endpoint = in->endpoint();
  endpoint->append_value(10);
  endpoint->append_value(20);
  quint8 tmp;
  // Read advances state
  REQUIRE_NOTHROW(in->read(0, {&tmp, 1}, rw));
  CHECK(tmp == 10);
  // Get does not modify current value.
  REQUIRE_NOTHROW(in->read(0, {&tmp, 1}, app));
  CHECK(tmp == 10);
  // Read advances state
  REQUIRE_NOTHROW(in->read(0, {&tmp, 1}, rw));
  CHECK(tmp == 20);

  // Soft-fail MMI, should yield default value
  in->setFailPolicy(sim::api2::memory::FailPolicy::YieldDefaultValue);
  REQUIRE_NOTHROW(in->read(0, {&tmp, 1}, rw));
  CHECK(tmp == 0);
  // Hard-fail MMI should throw
  in->setFailPolicy(sim::api2::memory::FailPolicy::RaiseError);
  REQUIRE_THROWS_AS(in->read(0, {&tmp, 1}, rw), sim::api2::memory::Error);
}
