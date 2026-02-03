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

#include "core/math/geom/interval.hpp"
#include <catch/catch.hpp>
#include "core/integers.h"

TEST_CASE("Interval Ops", "[scope:core][scope:core.math][kind:unit][arch:*]") {
  using namespace pepp::core;
  const Interval<i16> i01(0, 1);
  const Interval<i16> i03(0, 3);
  const Interval<i16> i05(0, 5);
  const Interval<i16> i11(1, 1);
  const Interval<i16> i14(1, 4);
  const Interval<i16> i23(2, 3);
  SECTION("Construction") {
    // REQUIRE_THROWS([]() { Interval<i16>(1, 0); }());
  }
  SECTION("Ordering") {

    CHECK(i11 == i11);
    CHECK(i11 < i14);
    CHECK(i11 < i23);
    CHECK(i11 > i05);

    CHECK(i14 > i11);
    CHECK(i14 == i14);
    CHECK(i14 < i23);
    CHECK(i14 > i05);

    CHECK(i23 > i11);
    CHECK(i23 > i14);
    CHECK(i23 == i23);
    CHECK(i23 > i05);

    CHECK(i05 < i11);
    CHECK(i05 < i14);
    CHECK(i05 < i23);
    CHECK(i05 == i05);
  }

  SECTION("Size") {
    CHECK(size_inclusive(i11) == 1);
    CHECK(size_exclusive(i11) == 0);
    CHECK(size_inclusive(i14) == 4);
    CHECK(size_exclusive(i14) == 3);
  }
  SECTION("Contains") {
    // antisymmetric
    CHECK(contains(i03, i11));
    CHECK(!contains(i11, i03));
    // reflexive
    CHECK(contains(i03, i03));
    // Requires overlap
    CHECK(!contains(i03, i14));
  }
  SECTION("Intersects") {
    // Symmetric
    CHECK(intersects(i03, i11));
    CHECK(intersects(i11, i03));
    // Requires overlap, but retains symmetry
    CHECK(!intersects(i11, i23));
    CHECK(!intersects(i23, i11));
  }
  SECTION("Intersection and Hull") {
    // Symmetric
    CHECK(i11 == intersection(i01, i14));
    CHECK(i11 == intersection(i14, i11));
    CHECK(Interval<i16>(0, 4) == hull(i01, i14));
    CHECK(i14 == hull(i14, i11));
    // Reflexive
    CHECK(i14 == intersection(i14, i14));
    CHECK(i14 == hull(i14, i14));
    // Idempotent if contained
    CHECK(i11 == intersection(i03, i11));
    CHECK(i03 == hull(i03, i11));
  }
  SECTION("Affine map") {
    // Idempotent
    CHECK(offset_map<i16>(2, i14, i14) == 2);
    //
    CHECK(offset_map<i16>(1, i14, Interval<i16>(2, 2)) == 2);
    // Invertible
    CHECK(offset_map(offset_map<i16>(2, i05, i14), i14, i05) == 2);
  }
}
