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

#include "core/math/geom/sparse_grid.hpp"
#include <catch/catch.hpp>
#include "core/integers.h"
#include "core/math/geom/interval.hpp"
#include "core/math/geom/rectangle.hpp"

TEST_CASE("Sparse Grid", "[scope:core][scope:core.math][kind:unit][arch:*]") {
  using namespace pepp::core;
  using Pt = Point<i16>;
  using Rect = Rectangle<i16>;
  using Ivl = Interval<i16>;
  using SG = SparseGrid;

  SECTION("Aligned 8x8 rect") {
    Rect r0(Ivl{0, 7}, Ivl{0, 7});
    SG grid;
    CHECK(grid.try_add(r0));
    CHECK(!grid.try_add(r0));
    CHECK(grid.overlap(r0));
    CHECK(grid.overlap(Pt{0, 0}));
    CHECK(!grid.overlap(Pt{-1, -1}));
    CHECK(grid.overlap(Pt{4, 4}));
    CHECK(grid.overlap(Pt{7, 7}));
    CHECK(!grid.overlap(Pt{8, 7}));
    CHECK(!grid.overlap(Pt{7, 8}));
    CHECK(!grid.overlap(Pt{8, 8}));
    grid.remove(r0);
    CHECK(!grid.overlap(Pt{0, 0}));
    CHECK(!grid.overlap(r0));
  }
  SECTION("2x2 rect at 7,7") {
    Rect r0(Ivl{7, 8}, Ivl{7, 8});
    SG grid;
    CHECK(grid.try_add(r0));
    CHECK(!grid.try_add(r0));
    CHECK(grid.overlap(r0));
    CHECK(!grid.overlap(Pt{0, 0}));
    CHECK(!grid.overlap(Pt{6, 6}));
    CHECK(grid.overlap(Pt{7, 7}));
    CHECK(grid.overlap(Pt{8, 7}));
    CHECK(grid.overlap(Pt{7, 8}));
    CHECK(grid.overlap(Pt{8, 8}));
    CHECK(grid.try_add(Pt{0, 0}));
    grid.remove(r0);
    CHECK(!grid.overlap(r0));
  }
}
