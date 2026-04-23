/*
 * Copyright (c) 2023-2026 J. Stanley Warford, Matthew McRaven
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

#include "core/compile/macro/macro_replacement.hpp"
#include <catch/catch.hpp>

TEST_CASE("Macro textual substitution", "[scope:core][scope:core.compile][kind:unit][arch:*]") {
  const auto test_string = "\\hello\\world";
  const auto with_numbers = "\\arg1\\arg2";

  SECTION("Basic out-of-bounds tests") {
    // No-replacements does not throw
    CHECK(pepp::tc::replace_macro_arguments(test_string, {}) == test_string);
    // Empty input does not throw.
    CHECK(pepp::tc::replace_macro_arguments("", {{"\\hello", "goodbye"}}) == "");
  }

  SECTION("Replacement with matches") {
    // No match
    CHECK(pepp::tc::replace_macro_arguments(test_string, {{"\\foo", "bar"}}) == test_string);
    // Single Match
    CHECK(pepp::tc::replace_macro_arguments(test_string, {{"\\hello", "goodbye"}}) == "goodbye\\world");
    CHECK(pepp::tc::replace_macro_arguments(test_string, {{"\\hello", "goodbye"}, {"\\world", "universe"}}) ==
          "goodbyeuniverse");
    // Matches can contain numbers
    CHECK(pepp::tc::replace_macro_arguments(with_numbers, {{"\\arg1", "foo"}, {"\\arg2", "bar"}}) == "foobar");
  }
  // Iterative matching happens when a macro defines a macro, which will be how .SCALL is implemented.
  SECTION("Iterative matches without seperators") {
    const auto input = "\\in\\outer";
    const auto first_pass = pepp::tc::replace_macro_arguments(input, {{"\\outer", "ner"}});
    CHECK(first_pass == "\\inner");
    const auto second_pass = pepp::tc::replace_macro_arguments(first_pass, {{"\\inner", "final"}});
    CHECK(second_pass == "final");
  }
  SECTION("Greediness") {
    CHECK(pepp::tc::replace_macro_arguments(test_string, {{"\\hel", "goodbye"}}) == test_string);
    CHECK(pepp::tc::replace_macro_arguments(with_numbers, {{"\\arg", "foo"}}) == with_numbers);
  }
  SECTION("Empty replacements") {
    CHECK(pepp::tc::replace_macro_arguments(test_string, {{"\\hello", ""}}) == "\\world");
    // \() works as a delimiter
    const auto input_1 = "\\arg1", input_2 = "\\arg\\()1";
    CHECK(pepp::tc::replace_macro_arguments(input_1, {{"\\arg1", "foo"}}) == "foo");
    CHECK(pepp::tc::replace_macro_arguments(input_2, {{"\\arg1", "foo"}, {"\\()", ""}}) == "\\arg1");
  }
}
