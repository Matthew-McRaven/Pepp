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
#include <catch.hpp>
#include "core/io/json_helpers.hpp"

TEST_CASE("JSON templatization", "[scope:core][scope:core.repl][kind:unit][arch:*]") {
  SECTION("Single substitution") {
    std::map<std::string, std::string> subs = {{"${name}", "World"}};
    // No subs
    CHECK("hello" == templatize("hello", subs));
    // Sub is whole string
    CHECK("World" == templatize("${name}", subs));
    // Sub is at end of string
    CHECK("hello World" == templatize("hello ${name}", subs));
    // Sub is in middle of string
    CHECK("hello World, today is nice!" == templatize("hello ${name}, today is nice!", subs));
    // Sub is repeated
    CHECK("WorldWorld" == templatize("${name}${name}", subs));
  }
  SECTION("Substitutions do not recruse") {
    std::map<std::string, std::string> subs = {{"${name}", "${cat}"}, {"${cat}", "World"}};
    // Sub is whole string
    CHECK("${cat}" == templatize("${name}", subs));
    // Sub is at end of string
    CHECK("hello ${cat}" == templatize("hello ${name}", subs));
    // Sub is in middle of string
    CHECK("hello ${cat}, today is nice!" == templatize("hello ${name}, today is nice!", subs));
    // Sub is repeated
    CHECK("${cat}${cat}" == templatize("${name}${name}", subs));
  }
}
