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

#include <catch/catch.hpp>
#include "cnl/scaled_integer.h"
#include "core/integers.h"
#include "core/math/geom/interval.hpp"
#include "core/math/geom/rectangle.hpp"

using namespace pepp::core;
using i16q2 = cnl::scaled_integer<i16, cnl::power<-2>>;
using Rect = Rectangle<i16q2>;
using Pt = Point<i16q2>;
using Ivl = Interval<i16q2>;
using RD = RectangleDecomposer<i16>;

TEST_CASE("Fixed-Point Rectangle Ops", "[scope:core][scope:core.math][kind:unit][arch:*]") {
  SECTION("Construction") {
    // Re-orders ranges as needed.
    CHECK_NOTHROW(Rect(Ivl{2, 2}, Ivl{-2, 4}) == Rect(Ivl{-2, 2}, Ivl{2, 4}));
    // Various static "constructors" work the same as the underlying constructor.
    CHECK(Rect(Pt{-2, 2}, Pt{2, 4}) == Rect::from_point_point(-2, 2, 2, 4));
    CHECK(Rect(Pt{-2, 2}, Pt{2.75, 4.75}) == Rect::from_point_size(-2, 2, 5, 3));
  }
  SECTION("Avoid epsilon/ 1 quantum errors for fractionals") {
    CHECK(Rect(Pt{-2, 2}, Pt{2.75, 4.75}) == Rect(Pt{-2, 2}, Size<i16q2>{5, 3}));
  }
  SECTION("Normalization") {
    auto a = Rect::from_point_point(2, 2, -2, -2);
    const auto b = Rect::from_point_point(-2, -2, 2, 2);
    CHECK(a != b);
    CHECK(!a.valid());
    CHECK(b.valid());
    CHECK(a.normalized() != a);
    CHECK(a.normalized() == b);
    a.normalize();
    CHECK(a == b);
    CHECK(b.normalized() == b);
  }
  SECTION("Accessors") {
    auto a = Rect::from_point_point(-3.25, -2.25, 4.5, 5.75);
    CHECK(a.left() == -3.25);
    CHECK(a.top() == -2.25);
    CHECK(a.right() == 4.5);
    CHECK(a.bottom() == 5.75);
  }
  SECTION("adjust") {
    auto start = Rect::from_point_point(1.25, 1.25, 3.25, 3.25);
    const auto end = Rect::from_point_point(0, 2, 5, 1);
    CHECK(start.adjusted(-1.25, +.75, +1.75, -2.25) == end);
  }
  SECTION("adjust turns into invalid") {
    auto start = Rect::from_point_point(1, 1, 1, 1);
    const auto end = Rect::from_point_point(2, 2, 1, 1);
    CHECK(start.adjusted(+1, +1, -1, -1) == end);
    CHECK(!end.valid());
  }
  SECTION("translate") {
    auto start = Rect::from_point_point(1, 1, 3, 3);
    const auto end = Rect::from_point_point(2, 0, 4, 2);
    CHECK(start.translated(+1, -1) == end);
    auto a = start;
    a.translate(+1, -1);
    CHECK(a == end);

    // Check that the point overloads work too.
    Point<i16q2> delta{1, -1};
    CHECK(start.translated(delta) == end);
    a = start;
    a.translate(delta);
    CHECK(a == end);
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
    CHECK(r1.width() == 11);
    CHECK(r1.height() == 6);
    CHECK(area(r1) == 66);
    CHECK(r2.width() == 11);
    CHECK(r2.height() == 6);
    CHECK(area(r2) == 66);
    CHECK(r3.width() == 3);
    CHECK(r3.height() == 2);
    CHECK(area(r3) == 6);
  }
  SECTION("Fractional Size") {
    Rect r(Ivl{0, 11.5}, Ivl{0, 6.25});
    CHECK(r.width() == 12.5);
    CHECK(r.height() == 7.25);
    CHECK(area(r) == 90.625);
  }
  SECTION("Contains") {
    auto p1 = Point<i16q2>{5, 2};
    auto p2 = Point<i16q2>{15, 2};
    auto p3 = Point<i16q2>{4, 1};

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

TEST_CASE("Fixed-Point Rectangle Decomposition", "[scope:core][scope:core.math][kind:unit][arch:*]") {
  SECTION("Aligned 1x1 rect @ 0,0") {
    const i16q2 max = 1.75;
    CHECK(to_rep(max) == 7);

    Rect r0(Ivl{0, max}, Ivl{0, max});
    const auto underlying = to_underlying_repr(r0);
    CHECK(underlying.x() == Interval<i16>{0, 7});
    CHECK(underlying.y() == Interval<i16>{0, 7});
    RD rd0(underlying);
    CHECK(std::distance(rd0.begin(), rd0.end()) == 1);
    auto begin = rd0.begin(), end = rd0.end();

    auto clip = Rectangle<u8>(Point<u8>{0, 0}, Size<u8>{8, 8});
    CHECK((*begin).first == Point<i16>{0, 0});
    CHECK((*begin).second == clip);
    begin++;

    CHECK(begin == end);
  }
  SECTION("Aligned 4x4 rect @ 0,0") {
    Rect r0(Ivl{0, 3}, Ivl{0, 3});
    const auto underlying = to_underlying_repr(r0);
    CHECK(underlying.x() == Interval<i16>{0, 3 * 4});
    CHECK(underlying.y() == Interval<i16>{0, 3 * 4});
    RD rd0(underlying);
    CHECK(std::distance(rd0.begin(), rd0.end()) == 4);
    auto begin = rd0.begin(), end = rd0.end();

    auto clip = Rectangle<u8>(Point<u8>{0, 0}, Size<u8>{8, 8});
    CHECK((*begin).first == Point<i16>{0, 0});
    CHECK((*begin).second == clip);
    begin++;

    clip = Rectangle<u8>(Point<u8>{0, 0}, Size<u8>{5, 8});
    CHECK((*begin).first == Point<i16>{8, 0});
    CHECK((*begin).second == clip);
    begin++;

    clip = Rectangle<u8>(Point<u8>{0, 0}, Size<u8>{8, 5});
    CHECK((*begin).first == Point<i16>{0, 8});
    CHECK((*begin).second == clip);
    begin++;

    clip = Rectangle<u8>(Point<u8>{0, 0}, Size<u8>{5, 5});
    CHECK((*begin).first == Point<i16>{8, 8});
    CHECK((*begin).second == clip);
    begin++;

    CHECK(begin == end);
  }
}
