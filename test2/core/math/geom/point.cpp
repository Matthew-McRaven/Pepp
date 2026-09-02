/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
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

#include "core/math/geom/point.hpp"
#include <catch/catch.hpp>
#include "core/integers.h"

TEST_CASE("Point Ops", "[scope:core][scope:core.math][kind:unit][arch:*]") {
  using namespace pepp::core;
  using Pt = Point<i16>;
  auto p1 = Pt{3, 4};
  auto p2 = Pt{5, 6};
  auto p3 = Pt{3, 6};
  SECTION("Scanline ordering") {
    CHECK(p1 == p1);
    CHECK(p1 < p2);
    CHECK(p1 < p3);

    CHECK(p2 > p1);
    CHECK(p2 == p2);
    CHECK(p2 > p3);

    CHECK(p3 > p1);
    CHECK(p3 < p2);
    CHECK(p3 == p3);
  }
}
