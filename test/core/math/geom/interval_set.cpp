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
}
