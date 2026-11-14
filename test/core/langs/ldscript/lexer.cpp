/*
 * Copyright (c) 2026. Stanley Warford, Matthew McRaven
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "core/langs/ldscript/lexer.hpp"
#include <catch.hpp>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>
#include "core/compile/lex/tokens.hpp"

namespace {
using pepp::tc::ldscript::Lexer;
using pepp::tc::support::SeekableData;

// Return the repr() of each token up to EoF.
std::vector<std::string> reprs(std::string text) {
  Lexer lexer(std::make_shared<std::unordered_set<std::string>>(), SeekableData(std::move(text)));
  std::vector<std::string> ret;
  do {
    ret.push_back(lexer.next_token()->repr());
    REQUIRE(ret.size() < 100); // Fail instead of looping forever if the lexer stops advancing.
  } while (ret.back() != "EoF()");
  return ret;
}
} // namespace

TEST_CASE("ld script lexer", "[scope:core][scope:core.langs][kind:unit][arch:*]") {
  using V = std::vector<std::string>;
  CHECK(reprs("x=1 *(.text.*) -lc # c\n/* d\n */ \"a b\" {}") ==
        V{"Identifier(x)", "Literal(=)", "Identifier(1)", "Identifier(*)", "Literal(()", "Identifier(.text.*)",
          "Literal())", "Identifier(-lc)", "InlineComment( c)", "BlockComment( d\n )", "StringConstant(a b)",
          "Literal({)", "Literal(})", "EoF()"});
  // An unclosed string or comment consumes the rest of the input.
  CHECK(reprs("a \"b c") == V{"Identifier(a)", "Invalid(\"b c)", "EoF()"});
  CHECK(reprs("a /* b") == V{"Identifier(a)", "Invalid(/* b)", "EoF()"});
  CHECK(pepp::tc::ldscript::is_unquoted_identifier(".text.*"));
  CHECK(!pepp::tc::ldscript::is_unquoted_identifier("a b"));
}
