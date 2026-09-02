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
#include <zpp_bits.h>
#include "sim3/api/traced/trace_packets.hpp"

TEST_CASE("Variable-length spans", "[scope:sim][kind:int][arch:*]") {
  using namespace sim::api2;
  auto [data, in, out] = zpp::bits::data_in_out();

  // Figure out  default-size of a packet payload.
  auto start = out.position();
  REQUIRE(out(packet::VariableBytes<32>(0)).code == std::errc());
  auto end = out.position();
  REQUIRE(end > start);
  auto size = end - start;

  out.reset();
  start = out.position();
  REQUIRE(out(packet::VariableBytes<32>(3)).code == std::errc());
  end = out.position();

  // Increasing the number of bytes in the value should increase the size of the packet.
  CHECK(end > start);
  CHECK((end - start) > size);
  REQUIRE((end - start) - size == 3);
}
