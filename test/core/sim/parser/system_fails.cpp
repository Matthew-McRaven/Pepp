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
#include <nlohmann/json.hpp>
#include "core/sim/system.hpp"
#include "core/sim/systemparser.hpp"

TEST_CASE("System Parser, System, Fails", "[scope:core][scope:core.sim][kind:unit][arch:*][!throws]") {
  using namespace bits;

  SECTION("incompatible type") {
    static const char *js = R"j({
      "compatible": "system,groot",
      "basename": "/"
    })j";
    ParsingContext ctx;
    REQUIRE_THROWS_AS(parse_system(js, ctx), ParsingError);
  }
  SECTION("name as int") {
    static const char *js = R"j({
      "basename": 5
    })j";
    ParsingContext ctx;
    REQUIRE_THROWS_AS(parse_system(js, ctx), ParsingError);
  }
  SECTION("compatible as int") {
    static const char *js = R"j({
      "compatible": 5
    })j";
    ParsingContext ctx;
    REQUIRE_THROWS_AS(parse_system(js, ctx), ParsingError);
  }
}
