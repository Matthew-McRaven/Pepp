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

#include "core/math/geom/spatial_map.hpp"
#include <catch/catch.hpp>
#include "core/integers.h"
#include "core/math/geom/interval.hpp"
#include "core/math/geom/occupancy_grid_sparse.hpp"
#include "core/math/geom/rectangle.hpp"

TEST_CASE("Spatial Map", "[scope:core][scope:core.math][kind:unit][arch:*]") {
  using namespace pepp::core;
  using Pt = Point<i16>;
  using Rect = Rectangle<i16>;
  using Ivl = Interval<i16>;
  using SG = SparseOccupancyGrid;
  using SM = SpatialMap;
  SECTION("Insert rectangle and test boundaries") {
    const Rect base(Ivl{0, 15}, Ivl{0, 15});
    SM map;
    auto id = map.try_add(base);
    CHECK(id.has_value());
    CHECK(!map.try_add(base).has_value());
    CHECK(map.overlap(base));
    // Simple point-wise overlaps
    CHECK(map.overlap(Pt{5, 5}));
    CHECK(map.overlap(Rect::from_point_size(-5, -5, 6, 6)));
    // Boundary checks
    // Top-left
    CHECK(!map.overlap(Pt{-1, -1}));
    CHECK(!map.overlap(Pt{0, -1}));
    CHECK(!map.overlap(Pt{-1, 0}));
    CHECK(map.overlap(Pt{0, 0}));
    // Top-right
    CHECK(!map.overlap(Pt{16, -1}));
    CHECK(!map.overlap(Pt{15, -1}));
    CHECK(!map.overlap(Pt{16, 0}));
    CHECK(map.overlap(Pt{15, 0}));
    // Bottom-left
    CHECK(!map.overlap(Pt{-1, 16}));
    CHECK(!map.overlap(Pt{0, 16}));
    CHECK(!map.overlap(Pt{-1, 15}));
    CHECK(map.overlap(Pt{0, 15}));
    // Bottom-right
    CHECK(!map.overlap(Pt{16, 16}));
    CHECK(!map.overlap(Pt{15, 16}));
    CHECK(!map.overlap(Pt{16, 15}));
    CHECK(map.overlap(Pt{15, 15}));
  }
  SECTION("Insert point and test boundaries") {
    const auto base = Pt{5, 5};
    SM map;
    auto id = map.try_add(base);
    CHECK(id.has_value());
    CHECK(!map.try_add(base).has_value());
    CHECK(map.overlap(base));
    CHECK(!map.overlap(Pt{4, 4}));
    CHECK(!map.overlap(Pt{5, 4}));
    CHECK(!map.overlap(Pt{6, 4}));
    CHECK(!map.overlap(Pt{4, 5}));
    CHECK(!map.overlap(Pt{6, 5}));
    CHECK(!map.overlap(Pt{4, 6}));
    CHECK(!map.overlap(Pt{5, 6}));
    CHECK(!map.overlap(Pt{6, 6}));
  }
  SECTION("Identifier from location") {
    const Rect base(Ivl{6, 7}, Ivl{6, 7});
    const Rect partial_overlap(Ivl{5, 6}, Ivl{5, 6});
    SM map;
    auto id = map.try_add(base);
    CHECK(id.has_value());
    CHECK(map.at(base) == id);
    CHECK(map.at(Pt{6, 6}) == id);
    CHECK(map.at(Pt{7, 6}) == id);
    CHECK(map.at(Pt{6, 7}) == id);
    CHECK(map.at(Pt{7, 7}) == id);
    CHECK(!map.at(Pt{5, 5}).has_value());
    CHECK(!map.at(partial_overlap).has_value());
    CHECK(map.overlap(partial_overlap));
    auto overlapping = map.overlapping(partial_overlap);
    CHECK(std::ranges::distance(overlapping) == 1);
    CHECK(overlapping.front().second == base);
  }
  SECTION("Relative movement") {
    const Rect base(Ivl{6, 7}, Ivl{6, 7});
    const Rect dest(Ivl{0, 1}, Ivl{0, 1});
    SM map;
    auto id = map.try_add(base);
    CHECK(id.has_value());
    CHECK(map.can_move_relative(*id, Pt{-6, -6}));
    map.move_relative(*id, Pt{-6, -6});
    CHECK(map.at(dest.top_left()) == id);
    CHECK(map.at(dest) == id);
  }
  SECTION("Absolute movement") {
    const Rect base(Ivl{6, 7}, Ivl{6, 7});
    const Rect dest(Ivl{0, 1}, Ivl{0, 1});
    SM map;
    auto id = map.try_add(base);
    CHECK(id.has_value());
    CHECK(map.can_move_absolute(*id, dest.top_left()));
    map.move_absolute(*id, dest.top_left());
    CHECK(map.at(dest.top_left()) == id);
    CHECK(map.at(dest) == id);
  }
  SECTION("add + remove") {
    const Rect base(Ivl{6, 7}, Ivl{6, 7});
    SM map;
    auto id = map.try_add(base);
    CHECK(id.has_value());
    CHECK(map.remove(*id));
    CHECK(!map.overlap(base));
    CHECK(!map.at(base).has_value());
    CHECK(map.bounding_box() == Rectangle<i16>{});
  }
  SECTION("Multiple overlaps") {
    const Rect base(Ivl{0, 18}, Ivl{7, 23});
    // Pt0 should miss (below Y threshold), as should pt3 (over x threshold); pt1 and pt2 should overlap.
    const Pt pt0{0, 5}, pt1{14, 9}, pt2{10, 10}, pt3{19, 15};
    SM map;
    auto id0 = map.try_add(pt0), id1 = map.try_add(pt1), id2 = map.try_add(pt2), id3 = map.try_add(pt3);
    CHECK(id0.has_value());
    CHECK(id1.has_value());
    CHECK(id2.has_value());
    CHECK(id3.has_value());
    CHECK(map.overlap(base));
    auto overlapping = map.overlapping(base);
    CHECK(std::ranges::distance(overlapping) == 2);
    auto it = overlapping.begin();
    CHECK(it->first == *id1);
    it++;
    CHECK(it->first == *id2);
  }
  SECTION("Dave's test case") {
    const auto first = Rect::from_point_size(2, 3, 4, 4);
    const auto second = Rect::from_point_size(2, 3, 1, 1);
    SM map;
    auto id1 = map.try_add(first);
    CHECK(id1.has_value());
    auto id2 = map.try_add(second);
    CHECK(!id2.has_value());
  }
  SECTION("Multi-move success") {
    const Rect r0(Ivl{0, 1}, Ivl{0, 1}), r1(Ivl{2, 3}, Ivl{0, 1}), r2(Ivl{4, 5}, Ivl{0, 1});
    SM map;
    auto id0 = map.try_add(r0), id1 = map.try_add(r1), id2 = map.try_add(r2);
    CHECK(id0.has_value());
    CHECK(id1.has_value());
    CHECK(id2.has_value());
    std::vector<u32> move{{id0.value(), id1.value(), id2.value()}};
    CHECK(map.can_move_relative(move, Pt{0, 0}));
    CHECK(map.move_relative(move, Pt{0, 0}));
    CHECK(map.at(r0) == id0);
    CHECK(map.at(r1) == id1);
    CHECK(map.at(r2) == id2);
    CHECK(map.can_move_relative(move, Pt{0, 2}));
    CHECK(map.move_relative(move, Pt{0, 2}));
    CHECK(map.at(r0.translated(Pt{0, 2})) == id0);
    CHECK(map.at(r1.translated(Pt{0, 2})) == id1);
    CHECK(map.at(r2.translated(Pt{0, 2})) == id2);
    // Only move a subset
    std::vector<u32> move2{{id0.value(), id2.value()}};
    // Would induce an overlap between r2 and r1.
    REQUIRE(!map.can_move_relative(move2, Pt{2, 0}));
    REQUIRE(map.can_move_relative(move2, Pt{-4, 0}));
    CHECK(map.move_relative(move2, Pt{-4, 0}));
    CHECK(map.at(r0.translated(Pt{-4, 2})) == id0);
    CHECK(map.at(r1.translated(Pt{0, 2})) == id1);
    CHECK(map.at(r2.translated(Pt{-4, 2})) == id2);
  }
  SECTION("Multi-move self-overlap") {
    const Rect r0(Ivl{0, 1}, Ivl{0, 1}), r1(Ivl{2, 3}, Ivl{2, 3});
    SM map;
    auto id0 = map.try_add(r0), id1 = map.try_add(r1);
    CHECK(id0.has_value());
    CHECK(id1.has_value());
    std::vector<u32> move{{id0.value(), id1.value()}};
    CHECK(map.can_move_relative(move, Pt{2, 2}));
    CHECK(map.move_relative(move, Pt{2, 2}));
    CHECK(map.at(r1) == id0);
    CHECK(map.at(r1.translated(Pt{2, 2})) == id1);
  }
  SECTION("Multi-move other-overlap") {
    const Rect r0(Ivl{0, 1}, Ivl{0, 1}), r1(Ivl{2, 3}, Ivl{2, 3});
    SM map;
    auto id0 = map.try_add(r0), id1 = map.try_add(r1);
    CHECK(id0.has_value());
    CHECK(id1.has_value());
    std::vector<u32> move{{id0.value()}};
    CHECK(!map.can_move_relative(move, Pt{2, 2}));
    CHECK(!map.move_relative(move, Pt{2, 2}));
    CHECK(map.at(r0) == id0);
    CHECK(map.at(r1) == id1);
  }
}
