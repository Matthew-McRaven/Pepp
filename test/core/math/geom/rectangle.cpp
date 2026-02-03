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

#include "core/math/geom/rectangle.hpp"
#include <catch/catch.hpp>
#include "core/integers.h"
#include "core/math/geom/interval.hpp"

TEST_CASE("Rectangle Ops", "[scope:core][scope:core.math][kind:unit][arch:*]") {
  using namespace pepp::core;
  using Rect = Rectangle<i16>;
  using Ivl = Interval<i16>;

  SECTION("Construction") {
    // Re-orders ranges as needed.
    CHECK_NOTHROW(Rect(Ivl{2, 2}, Ivl{-2, 4}) == Rect(Ivl{-2, 2}, Ivl{2, 4}));
  }
  // 1 overlaps 2, 1 contains 3. 2 does not overlap 3.
  Rect r1(Ivl{0, 10}, Ivl{0, 5});
  Rect r2(Ivl{5, 15}, Ivl{2, 7});
  Rect r3(Ivl{2, 4}, Ivl{0, 1});
  SECTION("Ordering") {
    CHECK(r1 == r1);
    CHECK(r1 < r2);
    CHECK(r1 < r3);

    CHECK(r2 > r1);
    CHECK(r2 == r2);
    CHECK(r2 > r3);

    CHECK(r3 > r1);
    CHECK(r3 < r2);
    CHECK(r3 == r3);
  }

  SECTION("Size") {
    CHECK(r1.width() == 10);
    CHECK(r1.height() == 5);
    CHECK(area(r1) == 50);
    CHECK(r2.width() == 10);
    CHECK(r2.height() == 5);
    CHECK(area(r2) == 50);
    CHECK(r3.width() == 2);
    CHECK(r3.height() == 1);
    CHECK(area(r3) == 2);
  }
  SECTION("Contains") {
    auto p1 = Point<i16>{5, 2};
    auto p2 = Point<i16>{15, 2};
    auto p3 = Point<i16>{4, 1};

    CHECK(contains(r1, p1));
    CHECK(!contains(r1, p2));
    CHECK(contains(r1, p3));
    CHECK(contains(r1, r1));
    CHECK(!contains(r1, r2));
    CHECK(contains(r1, r3));

    CHECK(contains(r2, p1));
    CHECK(contains(r2, p2));
    CHECK(!contains(r2, p3));
    CHECK(!contains(r2, r1));
    CHECK(contains(r2, r2));
    CHECK(!contains(r2, r3));

    CHECK(!contains(r3, p1));
    CHECK(!contains(r3, p2));
    CHECK(contains(r3, p3));
    CHECK(!contains(r3, r1));
    CHECK(!contains(r3, r2));
    CHECK(contains(r3, r3));
  }
  SECTION("Intersects") {
    // Check for reflexiveness and symmetry.
    CHECK(intersects(r1, r1));
    CHECK(intersects(r1, r2));
    CHECK(intersects(r1, r3));

    CHECK(intersects(r2, r1));
    CHECK(intersects(r2, r2));
    CHECK(!intersects(r2, r3));

    CHECK(intersects(r3, r1));
    CHECK(!intersects(r3, r2));
    CHECK(intersects(r3, r3));
  }
  SECTION("Intersection and Hull") {
    // Check for reflexiveness.
    CHECK(r1 == intersection(r1, r1));
    CHECK(r1 == hull(r1, r1));
    CHECK(r2 == intersection(r2, r2));
    CHECK(r2 == hull(r2, r2));
    CHECK(r3 == intersection(r3, r3));
    CHECK(r3 == hull(r3, r3));

    CHECK(intersection(r1, r2) == Rect(Ivl{5, 10}, Ivl{2, 5}));
    CHECK(hull(r1, r2) == Rect(Ivl{0, 15}, Ivl{0, 7}));

    CHECK(intersection(r1, r3) == Rect(Ivl{2, 4}, Ivl{0, 1}));
    CHECK(hull(r1, r3) == r1);
  }
}
