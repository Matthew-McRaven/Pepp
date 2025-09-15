
/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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

#include "toolchain2/parser/macro/parse.hpp"
#include <catch.hpp>
#include <tuple>
using namespace Qt::StringLiterals;
TEST_CASE("Macro parser", "[scope:macro][kind:unit][arch:*][tc2]") {
  SECTION("Accepts valid spaces") {
    CHECK(std::get<0>(macro::analyze_macro_definition(u"@deci 0"_s)));
    CHECK(std::get<0>(macro::analyze_macro_definition(u"@deci 	0"_s)));
    CHECK(std::get<0>(macro::analyze_macro_definition(u"@deci 0	"_s)));
    CHECK(std::get<0>(macro::analyze_macro_definition(u"@deci  0"_s)));
    CHECK(std::get<0>(macro::analyze_macro_definition(u"@deci  0  "_s)));
    CHECK(std::get<0>(macro::analyze_macro_definition(u"@deci	0	"_s)));
    CHECK(std::get<0>(macro::analyze_macro_definition(u" @deci 0"_s)));
  }
  SECTION("Rejects invalid spaces") {
    CHECK_FALSE(std::get<0>(macro::analyze_macro_definition(u"@deco​0​"_s))); // 0-width space before and after
    CHECK_FALSE(
        std::get<0>(macro::analyze_macro_definition(u"@ deci 0"_s))); // Can't have whitespace between @ and name
  }
  SECTION("Handles different arity") {
    auto _0ar = macro::analyze_macro_definition(u"@deci 0"_s);
    auto _8ar = macro::analyze_macro_definition(u"@deco 8"_s);
    REQUIRE(std::get<0>(_0ar));
    CHECK(std::get<1>(_0ar).toUtf8().toStdString() == "DECI");
    CHECK(std::get<2>(_0ar) == 0);
    REQUIRE(std::get<0>(_8ar));
    CHECK(std::get<1>(_8ar).toUtf8().toStdString() == "DECO");
    CHECK(std::get<2>(_8ar) == 8);
  }
  SECTION("Rejects comments") { CHECK_FALSE(std::get<0>(macro::analyze_macro_definition("@deci 2 ;fail"))); }
  SECTION("Ignores case") {
    auto lower = macro::analyze_macro_definition(u"@deci 0"_s);
    auto mixed = macro::analyze_macro_definition(u"@DeCi 0"_s);
    auto upper = macro::analyze_macro_definition(u"@DECI 0"_s);
    CHECK(lower == mixed);
    CHECK(mixed == upper);
  }
}
