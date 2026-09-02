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

#include "sim3/subsystems/ram/broadcast/mmo.hpp"

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

TEST_CASE("Memory-mapped output write", "[scope:sim][kind:int][arch:*]") {
  auto out = QSharedPointer<sim::memory::Output<quint16>>::create(desc, span, 0);
  CHECK(out->deviceID() == desc.id);
  auto endpoint = out->endpoint();
  quint8 tmp = 10;
  REQUIRE_NOTHROW(out->write(0, {&tmp, 1}, rw));
  tmp = 20;
  REQUIRE_NOTHROW(out->write(0, {&tmp, 1}, rw));
  auto _1 = endpoint->next_value();
  REQUIRE(_1.has_value());
  CHECK(*_1 == 10);
  auto _2 = endpoint->next_value();
  REQUIRE(_2.has_value());
  CHECK(*_2 == 20);

  // Application writes to MMO are no-ops, therefor no additional values to consume.
  tmp = 30;
  REQUIRE_NOTHROW(out->write(0, {&tmp, 1}, app));
  CHECK(endpoint->at_end());
  auto _3 = endpoint->current_value();
  REQUIRE(_3.has_value());
  CHECK(*_3 == 20);
}
