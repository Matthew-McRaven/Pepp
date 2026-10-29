/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
 *
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

#include "core/ds/glob/glob.hpp"
#include <catch.hpp>
#include <string>
#include "core/ds/glob/aho_corasick.hpp"

namespace {
// Find str in a glob holding only pattern, with value 1.
i64 match_one(std::string_view pattern, std::string_view str) {
  Glob glob;
  REQUIRE(glob.add(pattern, 1));
  return glob.find(str);
}
} // namespace

// Test cases for various string matching utilities using ELF section names as test data.
TEST_CASE("AhoCorasick", "[scope:core][scope:core.ds][kind:unit][arch:*]") {
  SECTION("Stars are optional and must be at start / end") {
    for (const auto pat : {".text", "*.cold", ".debug_*", "*.rel.ro*"}) CHECK(AhoCorasick::can_handle(pat));
    for (const auto pat : {"", "**", "*crtbegin?.o", ".text.*_unlikely", "\\*ABS\\*"})
      CHECK_FALSE(AhoCorasick::can_handle(pat));
    // No matches by default.
    CHECK(AhoCorasick().find(".text") == -1);
  }
  SECTION("Stars act as anchors") {
    AhoCorasick ac;
    REQUIRE(ac.add(".text", 1));
    REQUIRE(ac.add("*.cold", 2));
    REQUIRE(ac.add(".debug_*", 3));
    ac.compile();
    CHECK(ac.find(".text") == 1);
    CHECK(ac.find(".rela.text") == -1);
    CHECK(ac.find(".text.main.cold") == 2);
    CHECK(ac.find(".text.cold.main") == -1);
    CHECK(ac.find(".debug_line") == 3);
    CHECK(ac.find("") == -1);
  }
  SECTION("Overlapping matches return largest value") {
    AhoCorasick ac;
    REQUIRE(ac.add("*he*", 1));
    REQUIRE(ac.add("*she*", 2));
    REQUIRE(ac.add("*hers*", 3));
    // A repeat keeps the larger value.
    REQUIRE(ac.add("*he*", 0));
    ac.compile();
    CHECK(ac.find("ahe") == 1);
    CHECK(ac.find("ushe") == 2);
    // Reaching "hers" requires falling back from "she" to "he".
    CHECK(ac.find("ushers") == 3);
    CHECK(ac.find("sh") == -1);
  }
}

TEST_CASE("Glob", "[scope:core][scope:core.ds][kind:unit][arch:*]") {
  SECTION("Empty and match all") {
    CHECK(Glob().empty());
    CHECK(Glob().find(".text") == -1);
    CHECK(match_one("*", "") == 1);
    CHECK(match_one("*", ".text.main.cold") == 1);
  }
  SECTION("Literals, prefixes, suffixes, and substrings") {
    CHECK(match_one(".text", ".text") == 1);
    CHECK(match_one(".text", ".text.main") == -1);
    // A dot is literal, so .text.* needs one and .text* does not.
    CHECK(match_one(".text.*", ".text.main") == 1);
    CHECK(match_one(".text.*", ".text") == -1);
    CHECK(match_one(".text.*", ".textual") == -1);
    CHECK(match_one(".text*", ".textual") == 1);
    CHECK(match_one("*crtend.o", "/usr/lib/crtend.o") == 1);
    CHECK(match_one("*.rel.ro*", ".data.rel.ro.local") == 1);
  }
  SECTION("Question marks and brackets match one byte") {
    CHECK(match_one("*crtbegin?.o", "/usr/lib/crtbeginS.o") == 1);
    CHECK(match_one("*crtbegin?.o", "crtbegin.o") == -1);
    CHECK(match_one(".srodata.cst[48]", ".srodata.cst8") == 1);
    CHECK(match_one(".srodata.cst[48]", ".srodata.cst16") == -1);
    // Both negation markers, and an escaped ] inside a bracket.
    CHECK(match_one(".init_array.[!0-9]*", ".init_array.foo") == 1);
    CHECK(match_one(".init_array.[^0-9]*", ".init_array.00100") == -1);
    CHECK(match_one("[\\]]", "]") == 1);
  }
  SECTION("Backslash escape metacharacters") {
    // objdump's pseudo-section name.
    CHECK(match_one("\\*ABS\\*", "*ABS*") == 1);
    CHECK(match_one("\\*ABS\\*", "xABSx") == -1);
  }
  SECTION("Inner stars backtrack") {
    CHECK(match_one(".text.*_unlikely", ".text.a.b_unlikely") == 1);
    CHECK(match_one(".text.*_unlikely", ".text_unlikely") == -1);
    CHECK(match_one(".text.*.cold*", ".text.a.cold.b.cold") == 1);
    CHECK(match_one(".text.*.cold*", ".text.cold") == -1);
    // The star is retried after each a
    CHECK(match_one("*a??b?", "aaaa") == -1);
    CHECK(match_one("*a??b?", "aaaxxbx") == 1);
  }
  SECTION("Reject malformed patterns") {
    Glob glob;
    CHECK_FALSE(glob.add(".srodata.cst[48", 1));
    CHECK_FALSE(glob.add(".text\\", 1));
    CHECK_FALSE(glob.add(".init_array.[9-0]", 1));
  }
  SECTION("Highest value breaks ties") {
    // One pattern of each kind: match all, prefix, exact, and general.
    Glob glob;
    REQUIRE(glob.add("*", 0));
    REQUIRE(glob.add(".text.*", 2));
    // Repeated pattern keeps highest value
    REQUIRE(glob.add(".text", 1));
    REQUIRE(glob.add(".text", 0));
    REQUIRE(glob.add(".text.*_unlikely", 3));
    CHECK(glob.find(".text.main_unlikely") == 3);
    CHECK(glob.find(".text.main") == 2);
    CHECK(glob.find(".text") == 1);
    CHECK(glob.find(".data") == 0);
  }
  SECTION("Parallel matching") {
    // 64 or more general patterns switch to using the bit-parallel NFA.
    Glob glob;
    for (i64 it = 0; it < 64; ++it) REQUIRE(glob.add(".text.fn" + std::to_string(it) + ".[hc]*", it));
    REQUIRE(glob.add(".text.*_unlikely", 100));
    CHECK(glob.find(".text.fn10.hot") == 10);
    CHECK(glob.find(".text.fn63.cold") == 63);
    CHECK(glob.find(".text.fn10.warm") == -1);
    CHECK(glob.find(".text.a.b_unlikely") == 100);
  }
}
