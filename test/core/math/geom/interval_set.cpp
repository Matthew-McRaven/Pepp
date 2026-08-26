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
#include <bitset>
#include <catch/catch.hpp>
#include <random>
#include <vector>
#include "core/math/geom/interval.hpp"

TEST_CASE("IntervalSet", "[scope:core][scope:core.math][kind:unit][arch:*]") {
  using IS = pepp::core::IntervalSet<uint16_t>;

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
  SECTION("contains on an empty set") {
    IS set;
    CHECK_FALSE(set.contains(0));
    CHECK_FALSE(set.contains(0xFFFF));
  }
  SECTION("contains within a single interval") {
    IS set;
    set.insert({5, 10});
    CHECK_FALSE(set.contains(4));
    CHECK(set.contains(5));
    CHECK(set.contains(7));
    CHECK(set.contains(10));
    CHECK_FALSE(set.contains(11));
  }
  SECTION("contains across multiple intervals") {
    IS set;
    set.insert({0, 2});
    set.insert({5, 10});
    set.insert({20, 20});
    REQUIRE(set.intervals().size() == 3);
    // Endpoints ok, but +- 1 fail.
    CHECK(set.contains(0));
    CHECK(set.contains(2));
    CHECK_FALSE(set.contains(3));

    CHECK_FALSE(set.contains(4));
    CHECK(set.contains(5));
    CHECK(set.contains(10));
    CHECK_FALSE(set.contains(11));

    CHECK_FALSE(set.contains(19));
    CHECK(set.contains(20));
    CHECK_FALSE(set.contains(21));
  }
  SECTION("contains at the extremes of the value type") {
    IS set;
    set.insert({0, 0});
    set.insert({0xFFFF, 0xFFFF});
    REQUIRE(set.intervals().size() == 2);
    CHECK(set.contains(0));
    CHECK_FALSE(set.contains(1));
    CHECK_FALSE(set.contains(0xFFFE));
    CHECK(set.contains(0xFFFF));
  }
}

namespace {
// Extract the positions of contiguous 1s (runs) into a bitset. Used to validate merging behavior of IntervalSet.
std::vector<pepp::core::Interval<uint8_t>> runs_of(const std::bitset<256> &bits) {
  std::vector<pepp::core::Interval<uint8_t>> out;
  for (int i = 0; i < 256;) {
    if (!bits[i]) {
      ++i;
      continue;
    }
    int start = i;
    while (i < 256 && bits[i]) ++i;
    out.emplace_back(uint8_t(start), uint8_t(i - 1));
  }
  return out;
}

} // namespace

TEST_CASE("IntervalSet agrees with a bitset oracle", "[scope:core][scope:core.math][kind:unit][arch:*]") {
  using IS = pepp::core::IntervalSet<uint8_t>;
  // Fixed seeds to ensure reproducible failures
  for (uint32_t seed : {1u, 7u, 12345u, 0xDEADBEEFu}) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> pick(0, 255), len(0, 16);
    IS set;
    std::bitset<256> oracle;
    // Check that the bitset agrees with the IntervalSet on each insert.
    for (int step = 0; step < 200; ++step) {
      int lo = pick(rng), hi = std::min(255, lo + len(rng));
      set.insert(uint8_t(lo), uint8_t(hi));
      for (int v = lo; v <= hi; ++v) oracle.set(v);
      REQUIRE(set.intervals() == runs_of(oracle));
    }
    // Check that final result did not add or remove any values.
    std::bitset<256> queried;
    for (int v = 0; v < 256; ++v) queried[v] = set.contains(uint8_t(v));
    REQUIRE(queried == oracle);
  }
}
