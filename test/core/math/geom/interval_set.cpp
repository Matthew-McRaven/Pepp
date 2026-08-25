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

#include "core/math/geom/interval_set.hpp"
#include <catch/catch.hpp>
#include "core/math/geom/interval.hpp"

TEST_CASE("Inclusive IntervalSet", "[scope:core][scope:core.math][kind:unit][arch:*]") {
  using IS = pepp::core::IntervalSet<uint16_t, true>;

  SECTION("Append-only, no merge") {
    IS set;
    set.insert({0, 0});
    CHECK(set.intervals().size() == 1);
    set.insert({2, 2});
    CHECK(set.intervals().size() == 2);
    set.insert({4, 4});
    CHECK(set.intervals().size() == 3);
  }
  SECTION("Clear") {
    IS set;
    set.insert({0, 0});
    set.insert({2, 2});
    set.insert({4, 4});
    REQUIRE(set.intervals().size() == 3);
    set.clear();
    CHECK(set.intervals().empty());
  }
  SECTION("Append-only and merge") {
    IS set;
    set.insert({0, 1});
    CHECK(set.intervals().size() == 1);
    set.insert({2, 3});
    CHECK(set.intervals().size() == 1);
    set.insert({4, 4});
    CHECK(set.intervals().size() == 1);
  }
  SECTION("Prepend-only, no merge") {
    IS set;
    set.insert({4, 4});
    CHECK(set.intervals().size() == 1);
    set.insert({2, 2});
    CHECK(set.intervals().size() == 2);
    set.insert({0, 0});
    CHECK(set.intervals().size() == 3);
  }
  SECTION("Prepend-only and merge") {
    IS set;
    set.insert({4, 4});
    CHECK(set.intervals().size() == 1);
    set.insert({2, 3});
    CHECK(set.intervals().size() == 1);
    set.insert({0, 1});
    CHECK(set.intervals().size() == 1);
  }
  SECTION("Merge previous and next intervals") {
    IS set;
    set.insert({0, 1});
    set.insert({3, 4});
    CHECK(set.intervals().size() == 2);
    set.insert({2, 4});
    CHECK(set.intervals().size() == 1);
  }
  SECTION("Insert contained in existing interval") {
    IS set;
    set.insert({0, 1});
    CHECK(set.intervals().size() == 1);
    set.insert(1);
    CHECK(set.intervals().size() == 1);
  }
  SECTION("Existing contained in inserted interval") {
    IS set;
    set.insert({0, 0});
    set.insert({2, 2});
    set.insert({4, 4});
    set.insert({6, 6});
    CHECK(set.intervals().size() == 4);
    set.insert({1, 5});
    CHECK(set.intervals().size() == 1);
  }
  SECTION("Wrap-around") {
    IS set;
    set.insert({0, 0xFFFD});
    CHECK(set.intervals().size() == 1);
    set.insert({0xFFFE, 0xFFFF});
    CHECK(set.intervals().size() == 1);
    auto i = *set.intervals().begin();
    CHECK(i.lower() == 0);
    CHECK(i.upper() == 0xffff);
  }
  SECTION("insert(lower, upper) overload") {
    IS set;
    set.insert(0, 1);
    CHECK(set.intervals().size() == 1);
    // [2, 3] is adjacent to [0, 1], so the inclusive set merges rather than appends.
    set.insert(2, 3);
    REQUIRE(set.intervals().size() == 1);
    auto merged = *set.intervals().begin();
    CHECK(merged.lower() == 0);
    CHECK(merged.upper() == 3);
    set.insert(5, 6);
    CHECK(set.intervals().size() == 2);
  }
  SECTION("insert(point) overload") {
    IS set;
    set.insert(4);
    REQUIRE(set.intervals().size() == 1);
    CHECK(set.intervals().begin()->lower() == 4);
    CHECK(set.intervals().begin()->upper() == 4);
    // Points on either side are adjacent, so all three collapse into one interval.
    set.insert(3);
    set.insert(5);
    REQUIRE(set.intervals().size() == 1);
    auto merged = *set.intervals().begin();
    CHECK(merged.lower() == 3);
    CHECK(merged.upper() == 5);
  }
  SECTION("Invalid intervals are ignored") {
    IS set;
    // Default-constructed intervals are empty, as are inverted ones. Neither describes any address.
    set.insert(pepp::core::Interval<uint16_t>());
    CHECK(set.intervals().empty());
    set.insert({5, 2});
    CHECK(set.intervals().empty());
  }
  SECTION("Invalid intervals do not perturb existing contents") {
    IS set;
    set.insert({0, 1});
    set.insert({4, 5});
    REQUIRE(set.intervals().size() == 2);
    set.insert(pepp::core::Interval<uint16_t>());
    set.insert({5, 2});
    REQUIRE(set.intervals().size() == 2);
    auto it = set.intervals().begin();
    CHECK(it->lower() == 0);
    CHECK(it->upper() == 1);
    ++it;
    CHECK(it->lower() == 4);
    CHECK(it->upper() == 5);
  }
  SECTION("Adjacency merges previous and next in one insert") {
    IS set;
    set.insert({0, 0});
    set.insert({2, 2});
    set.insert({4, 4});
    REQUIRE(set.intervals().size() == 3);
    // [1, 1] is adjacent to [0, 0] below and [2, 2] above; [4, 4] is two away and must survive.
    set.insert({1, 1});
    REQUIRE(set.intervals().size() == 2);
    auto it = set.intervals().begin();
    CHECK(it->lower() == 0);
    CHECK(it->upper() == 2);
    ++it;
    CHECK(it->lower() == 4);
    CHECK(it->upper() == 4);
  }
  SECTION("Re-inserting an identical interval is idempotent") {
    IS set;
    set.insert({2, 4});
    REQUIRE(set.intervals().size() == 1);
    for (int rep = 0; rep < 3; ++rep) set.insert({2, 4});
    REQUIRE(set.intervals().size() == 1);
    auto only = *set.intervals().begin();
    CHECK(only.lower() == 2);
    CHECK(only.upper() == 4);
  }
}

TEST_CASE("Exclusive IntervalSet", "[scope:core][scope:core.math][kind:unit][arch:*]") {
  using ISE = pepp::core::IntervalSet<uint16_t, false>;
  SECTION("Append-only, no merge") {
    ISE set;
    set.insert({0, 1});
    CHECK(set.intervals().size() == 1);
    set.insert({2, 3});
    CHECK(set.intervals().size() == 2);
    set.insert({4, 5});
    CHECK(set.intervals().size() == 3);
  }
  SECTION("Clear") {
    ISE set;
    set.insert({0, 0});
    set.insert({2, 2});
    set.insert({4, 4});
    REQUIRE(set.intervals().size() == 3);
    set.clear();
    CHECK(set.intervals().empty());
  }
  SECTION("Append-only and merge") {
    ISE set;
    set.insert({0, 1});
    CHECK(set.intervals().size() == 1);
    set.insert({1, 2});
    CHECK(set.intervals().size() == 1);
    set.insert({2, 3});
    CHECK(set.intervals().size() == 1);
  }
  SECTION("Prepend-only, no merge") {
    ISE set;
    set.insert({4, 5});
    CHECK(set.intervals().size() == 1);
    set.insert({2, 3});
    CHECK(set.intervals().size() == 2);
    set.insert({0, 1});
    CHECK(set.intervals().size() == 3);
  }
  SECTION("Prepend-only and merge") {
    ISE set;
    set.insert({3, 4});
    CHECK(set.intervals().size() == 1);
    set.insert({2, 3});
    CHECK(set.intervals().size() == 1);
    set.insert({0, 2});
    CHECK(set.intervals().size() == 1);
  }
  SECTION("Merge next intervals") {
    ISE set;
    set.insert({0, 1});
    set.insert({3, 4});
    CHECK(set.intervals().size() == 2);
    set.insert({2, 4});
    CHECK(set.intervals().size() == 2);
  }
  SECTION("Insert contained in existing interval") {
    ISE set;
    set.insert({0, 1});
    CHECK(set.intervals().size() == 1);
    set.insert(1);
    CHECK(set.intervals().size() == 1);
  }
  SECTION("Existing contained in inserted interval") {
    ISE set;
    set.insert({0, 0});

    set.insert({2, 2});
    set.insert({4, 4});

    set.insert({6, 6});
    CHECK(set.intervals().size() == 4);
    set.insert({1, 5});
    CHECK(set.intervals().size() == 3);
  }
  SECTION("Wrap-around") {
    ISE set;
    set.insert({0, 0xFFFD});
    CHECK(set.intervals().size() == 1);
    set.insert({0xFFFD, 0xFFFF});
    CHECK(set.intervals().size() == 1);
    auto i = *set.intervals().begin();
    CHECK(i.lower() == 0);
    CHECK(i.upper() == 0xffff);
  }
  SECTION("insert(lower, upper) overload") {
    ISE set;
    set.insert(0, 1);
    CHECK(set.intervals().size() == 1);
    // Exclusive sets treat [0, 1) and [1, 2) as adjacent, so these merge.
    set.insert(1, 2);
    REQUIRE(set.intervals().size() == 1);
    auto merged = *set.intervals().begin();
    CHECK(merged.lower() == 0);
    CHECK(merged.upper() == 2);
    set.insert(4, 5);
    CHECK(set.intervals().size() == 2);
  }
  SECTION("insert(point) overload") {
    ISE set;
    set.insert(4);
    REQUIRE(set.intervals().size() == 1);
    CHECK(set.intervals().begin()->lower() == 4);
    CHECK(set.intervals().begin()->upper() == 4);
  }
  SECTION("Invalid intervals are ignored") {
    ISE set;
    // Default-constructed intervals are empty, as are inverted ones. Neither describes any address.
    set.insert(pepp::core::Interval<uint16_t>());
    CHECK(set.intervals().empty());
    set.insert({5, 2});
    CHECK(set.intervals().empty());
  }
  SECTION("Invalid intervals do not perturb existing contents") {
    ISE set;
    set.insert({0, 1});
    set.insert({4, 5});
    REQUIRE(set.intervals().size() == 2);
    set.insert(pepp::core::Interval<uint16_t>());
    set.insert({5, 2});
    REQUIRE(set.intervals().size() == 2);
    auto it = set.intervals().begin();
    CHECK(it->lower() == 0);
    CHECK(it->upper() == 1);
    ++it;
    CHECK(it->lower() == 4);
    CHECK(it->upper() == 5);
  }
  SECTION("Adjacency merges previous and next in one insert") {
    ISE set;
    set.insert({0, 1});
    set.insert({2, 3});
    set.insert({5, 6});
    REQUIRE(set.intervals().size() == 3);
    // [1, 2) closes the gap between [0, 1) and [2, 3); [5, 6) is disjoint and must survive.
    set.insert({1, 2});
    REQUIRE(set.intervals().size() == 2);
    auto it = set.intervals().begin();
    CHECK(it->lower() == 0);
    CHECK(it->upper() == 3);
    ++it;
    CHECK(it->lower() == 5);
    CHECK(it->upper() == 6);
  }
  SECTION("Re-inserting an identical interval is idempotent") {
    ISE set;
    set.insert({2, 4});
    REQUIRE(set.intervals().size() == 1);
    for (int rep = 0; rep < 3; ++rep) set.insert({2, 4});
    REQUIRE(set.intervals().size() == 1);
    auto only = *set.intervals().begin();
    CHECK(only.lower() == 2);
    CHECK(only.upper() == 4);
  }
}
