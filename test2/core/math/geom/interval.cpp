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
  const Interval<i16> empty{};
  SECTION("Swapped") {
    auto start = i14, end = Interval<i16>(4, 1);
    CHECK(start != end);
    CHECK(start.valid());
    CHECK(!end.valid());
    CHECK(start == end.flipped());
    CHECK(start.flipped() == end);
    CHECK(start.normalized() == start);
    CHECK(end.normalized() == start);
  }
  SECTION("Ordering") {
    CHECK(i11 == i11);
    CHECK(i11 < i14);
    CHECK(i11 < i23);
    CHECK(i11 > i05);
    CHECK(i11 > empty);

    CHECK(i14 > i11);
    CHECK(i14 == i14);
    CHECK(i14 < i23);
    CHECK(i14 > i05);
    CHECK(i14 > empty);

    CHECK(i23 > i11);
    CHECK(i23 > i14);
    CHECK(i23 == i23);
    CHECK(i23 > i05);
    CHECK(i23 > empty);

    CHECK(i05 < i11);
    CHECK(i05 < i14);
    CHECK(i05 < i23);
    CHECK(i05 == i05);
    CHECK(i05 > empty);

    CHECK(empty < i11);
    CHECK(empty < i14);
    CHECK(empty < i23);
    CHECK(empty < i05);
    CHECK(empty == empty);
  }

  SECTION("Size") {
    CHECK(size_inclusive(i11) == 1);
    CHECK(size_exclusive(i11) == 0);
    CHECK(size_inclusive(i14) == 4);
    CHECK(size_exclusive(i14) == 3);
    CHECK(size_inclusive(empty) == 0);
    CHECK(size_exclusive(empty) == 0);
  }
  SECTION("Contains") {
    // antisymmetric
    CHECK(contains(i03, i11));
    CHECK(!contains(i11, i03));
    CHECK(!contains(empty, i03));
    CHECK(!contains(i03, empty));
    // reflexive
    CHECK(contains(i03, i03));
    CHECK(!contains(empty, empty));
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
    CHECK(!intersects(empty, i03));
    CHECK(!intersects(i03, empty));
    CHECK(!intersects(empty, empty));
    // Empty is a zero and not asymmetric
    CHECK(!intersects(empty, i14));
    CHECK(!intersects(empty, empty));
  }
  SECTION("Intersection and Hull") {
    // Symmetric
    CHECK(i11 == intersection(i01, i14));
    CHECK(i11 == intersection(i14, i11));
    CHECK(Interval<i16>(0, 4) == hull(i01, i14));
    CHECK(i14 == hull(i14, i11));
    // Intersection requries overlap
    CHECK(empty == intersection(i01, i23));

    // Reflexive
    CHECK(i14 == intersection(i14, i14));
    CHECK(i14 == hull(i14, i14));
    // Idempotent if contained
    CHECK(i11 == intersection(i03, i11));
    CHECK(i03 == hull(i03, i11));

    // Empty is identity for hull, and zero for intersection
    CHECK(empty == intersection(empty, i23));
    CHECK(i23 == hull(empty, i23));
    CHECK(empty == intersection(empty, empty));
    CHECK(empty == hull(empty, empty));
  }
  SECTION("Affine map") {
    // Idempotent
    CHECK(offset_map<i16>(2, i14, i14) == 2);
    //
    CHECK(offset_map<i16>(1, i14, Interval<i16>(2, 2)) == 2);
    // Invertible
    CHECK(offset_map(offset_map<i16>(2, i05, i14), i14, i05) == 2);
  }
  SECTION("Approximate midpoint") { CHECK(i14.midpoint_approximate() == 2); }
}
