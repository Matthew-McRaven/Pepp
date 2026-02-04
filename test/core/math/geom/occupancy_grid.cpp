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

#include "core/math/geom/occupancy_grid.hpp"
#include <catch/catch.hpp>
#include <sstream>
#include "core/integers.h"
#include "core/math/geom/point.hpp"
#include "core/math/geom/rectangle.hpp"

TEST_CASE("OccupancyGrid", "[scope:core][scope:core.math][kind:unit][arch:*]") {
  using namespace pepp::core;
  using Pt = Point<u8>;
  using Ival = Interval<u8>;
  using Rect = Rectangle<u8>;
  SECTION("Construct empty / full grids") {
    auto empty1 = OccupancyGrid();
    CHECK(empty1.empty());
    CHECK(empty1.count() == 0);
    auto empty2 = OccupancyGrid::zeroes();
    CHECK(empty2.empty());
    CHECK(empty2.count() == 0);
    CHECK(empty1 == empty2);

    auto full1 = OccupancyGrid::ones();
    CHECK(full1.full());
    CHECK(full1.count() == 64);
  }

  SECTION("Construct from Point") {
    {
      std::ostringstream ss;
      ss << OccupancyGrid(Point<u8>(2, 3));
      auto res = "00000000\n00000000\n00000000\n00100000\n00000000\n00000000\n00000000\n00000000\n";
      CHECK(ss.str() == res);
    }

    {
      std::ostringstream ss;
      auto og = OccupancyGrid() | Pt(0, 0) | Pt(7, 0) | Pt(7, 7) | Pt(0, 7);
      ss << og;
      auto res = "10000001\n00000000\n00000000\n00000000\n00000000\n00000000\n00000000\n10000001\n";
      CHECK(ss.str() == res);
      CHECK(og.count() == 4);
      CHECK(!og.empty());
      CHECK(!og.full());
    }
  }

  SECTION("Construct from Rectangle") {
    {
      Ival x(2, 6);
      Ival y(1, 2);
      Rect r(x, y);
      std::ostringstream ss;
      auto og = OccupancyGrid(r);
      ss << og;
      auto res = "00000000\n00111110\n00111110\n00000000\n00000000\n00000000\n00000000\n00000000\n";
      CHECK(ss.str() == res);
      CHECK((OccupancyGrid{} | r) == og);
      CHECK(og.row_bits(0) == 0);
      CHECK(og.row_bits(1) == 0b01111100);
      CHECK(og.row_bits(2) == 0b01111100);
      CHECK(og.count() == 10);
    }
  }

  SECTION("Check column_bits") {
    for (int col = 0; col < 8; col++) {
      for (int it = 1; it < 256; it++) {
        OccupancyGrid og(0);
        if (it & 0x01) og |= Pt(col, 0);
        if (it & 0x02) og |= Pt(col, 1);
        if (it & 0x04) og |= Pt(col, 2);
        if (it & 0x08) og |= Pt(col, 3);
        if (it & 0x10) og |= Pt(col, 4);
        if (it & 0x20) og |= Pt(col, 5);
        if (it & 0x40) og |= Pt(col, 6);
        if (it & 0x80) og |= Pt(col, 7);
        CHECK(og.column_bits(col) == it);
        CHECK(og.row_bits(0) == ((it & 0x01) ? 1 << col : 0));
        CHECK(og.row_bits(1) == ((it & 0x02) ? 1 << col : 0));
        CHECK(og.row_bits(2) == ((it & 0x04) ? 1 << col : 0));
        CHECK(og.row_bits(3) == ((it & 0x08) ? 1 << col : 0));
        CHECK(og.row_bits(4) == ((it & 0x10) ? 1 << col : 0));
        CHECK(og.row_bits(5) == ((it & 0x20) ? 1 << col : 0));
        CHECK(og.row_bits(6) == ((it & 0x40) ? 1 << col : 0));
        CHECK(og.row_bits(7) == ((it & 0x80) ? 1 << col : 0));
        CHECK(og.count() == std::popcount(static_cast<u64>(it)));
        // Equivalence of the helper method and the manual one
        // Start with all 1's to prove that masking out existing bits works
        auto set_col = OccupancyGrid::ones();
        set_col.set_column_bits(col, it);
        for (int jt = 0; jt < 8; jt++) {
          if (jt != col) set_col.set_column_bits(jt, 0);
        }
        CHECK(og == set_col);
      }
    }
  }
  SECTION("Arithmetic Operators") {
    OccupancyGrid og(0xFEEDDEADBEEFCAFEuLL);
    // Identities
    CHECK(og == (OccupancyGrid::ones() & og));
    CHECK(og == (OccupancyGrid::zeroes() | og));
    CHECK(og == (OccupancyGrid::zeroes() ^ og));
    CHECK(OccupancyGrid::zeroes() == (og ^ og));
    CHECK(og - OccupancyGrid::ones() == OccupancyGrid::zeroes());
    CHECK(OccupancyGrid::ones() - og == ~og);
  }
  SECTION("Translation") {
    static const u8 row_bits = 0b0100'1110;
    auto og = OccupancyGrid{}.with_row_bits(2, row_bits);
    CHECK(og.row_bits(2) == row_bits);
    {
      auto shifted_left = og.shift_left(2);
      CHECK(shifted_left.row_bits(2) == static_cast<u8>(row_bits << 2));
    }
    {
      auto shifted_right = og.shift_right(2);
      CHECK(shifted_right.row_bits(2) == static_cast<u8>(row_bits >> 2));
    }
    {
      auto shifted_up = og.shift_up(2);
      CHECK(shifted_up.row_bits(0) == row_bits);
    }
    {
      auto shifted_down = og.shift_down(2);
      CHECK(shifted_down.row_bits(4) == row_bits);
    }
  }
  SECTION("Transpose & Flip") {
    Ival x(2, 6);
    Ival y(1, 2);
    Rect r(x, y);
    auto og = OccupancyGrid(r);
    CHECK(og.count() == 10);
    {
      std::ostringstream ss;
      ss << og.mirror_y().mirror_y();
      auto res = "00000000\n00111110\n00111110\n00000000\n00000000\n00000000\n00000000\n00000000\n";
      CHECK(ss.str() == res);
    }
    // Transpose
    {
      std::ostringstream ss;
      ss << og.transpose();
      auto res = "00000000\n00000000\n01100000\n01100000\n01100000\n01100000\n01100000\n00000000\n";
      CHECK(ss.str() == res);
    }
    // Mirror across X axis
    {
      std::ostringstream ss;
      ss << og.mirror_x();
      auto res = "00000000\n00000000\n00000000\n00000000\n00000000\n00111110\n00111110\n00000000\n";
      CHECK(ss.str() == res);
    }
    // Mirror across Y axis
    {
      std::ostringstream ss;
      ss << og.mirror_y();
      auto res = "00000000\n01111100\n01111100\n00000000\n00000000\n00000000\n00000000\n00000000\n";
      CHECK(ss.str() == res);
    }
  }
}
