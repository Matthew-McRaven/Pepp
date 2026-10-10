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
#include "core/langs/expr/lexer.hpp"
#include <catch.hpp>
#include <string>
#include <unordered_set>
#include <vector>
#include "core/compile/lex/tokens.hpp"
#include "core/langs/asmb/asmb_tokens.hpp"

namespace {
namespace lex = pepp::tc::lex;
using pepp::tc::expr::ExpressionLexer;
using pepp::tc::support::Location;
using pepp::tc::support::SeekableData;

// Lex until EoF. Identifier tokens point into the pool, so it must outlive them.
std::vector<std::shared_ptr<lex::Token>> lex_all(std::shared_ptr<std::unordered_set<std::string>> pool, std::string text,
                                                 Location origin = Location(0, 0),
                                                 pepp::tc::expr::Features features = {}) {
  ExpressionLexer lexer(pool, SeekableData(std::move(text), origin), features);
  std::vector<std::shared_ptr<lex::Token>> ret;
  do {
    ret.push_back(lexer.next_token());
    REQUIRE(ret.back() != nullptr);
    REQUIRE(ret.size() < 100); // Guarantee forward progress.
  } while (ret.back()->type() != lex::EoF::TYPE);
  return ret;
}

// The repr() of each token, so a whole token stream can be compared at once.
std::vector<std::string> reprs(const std::string &text, pepp::tc::expr::Features features = {}) {
  auto pool = std::make_shared<std::unordered_set<std::string>>();
  std::vector<std::string> ret;
  for (const auto &token : lex_all(pool, text, Location(0, 0), features)) ret.push_back(token->repr());
  return ret;
}
} // namespace

TEST_CASE("Expression lexer", "[scope:core][scope:core.langs][kind:unit][arch:*]") {
  using V = std::vector<std::string>;
  SECTION("full-stop/. depends on the lexer's options") {
    using Dot = pepp::tc::expr::Features::Dot;
    // Disabled
    CHECK(reprs("a.b .") == V{"Identifier(a)", "Invalid(.)", "Identifier(b)", "Invalid(.)", "EoF()"});
    // As an identifier character
    CHECK(reprs("a.b .", {Dot::Identifier}) ==
          V{"Identifier(a)", "Literal(.)", "Identifier(b)", "Literal(.)", "EoF()"});
    // As an operator
    CHECK(reprs("a.b", {Dot::Operator}) == V{"Identifier(a)", "Literal(.)", "Identifier(b)", "EoF()"});
  }
  SECTION("Various integers") {
    CHECK(reprs("0 42 0x1F 0XfF") == V{"Integer(0)", "Integer(42)", "Integer(0x1F)", "Integer(0xFF)", "EoF()"});
    CHECK(reprs("18446744073709551615 0xFFFFFFFFFFFFFFFF") ==
          V{"Integer(18446744073709551615)", "Integer(0xFFFFFFFFFFFFFFFF)", "EoF()"});
    // Signs are operators, not part of the integer.
    CHECK(reprs("-5 +5") == V{"Literal(-)", "Integer(5)", "Literal(+)", "Integer(5)", "EoF()"});
    // Like the assembler's lexer, a digit run ends at the first non-digit.
    CHECK(reprs("12ab") == V{"Integer(12)", "Identifier(ab)", "EoF()"});
  }
  SECTION("All operators") {
    CHECK(reprs("<< >> <= >= == != && || + - * / % < > & ^ | ~ ! ( )") ==
          V{"Literal(<<)", "Literal(>>)", "Literal(<=)", "Literal(>=)", "Literal(==)", "Literal(!=)", "Literal(&&)", "Literal(||)", "Literal(+)", "Literal(-)", "Literal(*)", "Literal(/)",
            "Literal(%)", "Literal(<)", "Literal(>)", "Literal(&)", "Literal(^)", "Literal(|)", "Literal(~)", "Literal(!)", "Literal(()", "Literal())", "EoF()"});
    CHECK(reprs("a<<<b") == V{"Identifier(a)", "Literal(<<)", "Literal(<)", "Identifier(b)", "EoF()"});
    CHECK(reprs("a&&&b") == V{"Identifier(a)", "Literal(&&)", "Literal(&)", "Identifier(b)", "EoF()"});
  }
  SECTION("Identifiers and character constants") {
    CHECK(reprs("foo _bar x2") == V{"Identifier(foo)", "Identifier(_bar)", "Identifier(x2)", "EoF()"});
    // The token holds the text between the quotes; escapes are decoded by bits::escapedToByte().
    CHECK(reprs(R"('a' '\n' '\x41' '\'')") == V{"CharacterConstant(a)", R"(CharacterConstant(\n))", R"(CharacterConstant(\x41))", R"(CharacterConstant(\'))", "EoF()"});
  }
  SECTION("An assortment of invalid tokens") {
    CHECK(reprs(R"(, ; " = $ #)") ==
          V{"Invalid(,)", "Invalid(;)", "Invalid(\")", "Invalid(=)", "Invalid($)", "Invalid(#)", "EoF()"});
    CHECK(reprs("0x") == V{"Invalid(0x)", "EoF()"});
    CHECK(reprs("18446744073709551616") == V{"Invalid(18446744073709551616)", "EoF()"}); // Exceeds 64 bits.
    CHECK(reprs("'ab'") == V{"Invalid(')", "Identifier(ab)", "Invalid(')", "EoF()"});
  }
  SECTION("Whitespace and newlines") {
    CHECK(reprs("") == V{"EoF()"});
    CHECK(reprs(" \t a \r\n b ") == V{"Identifier(a)", "Empty()", "Identifier(b)", "EoF()"});
  }
  SECTION("Locations are relative to the start of the input") {
    auto pool = std::make_shared<std::unordered_set<std::string>>();
    const auto tokens = lex_all(pool, "  ab+0x10", Location(2, 5));
    REQUIRE(tokens.size() == 4);
    CHECK(tokens[0]->location().lower() == Location(2, 7)); // ab
    CHECK(tokens[0]->location().upper() == Location(2, 9));
    CHECK(tokens[1]->location().lower() == Location(2, 9)); // +
    CHECK(tokens[2]->location().lower() == Location(2, 10)); // 0x10
    CHECK(tokens[2]->location().upper() == Location(2, 14));
  }
}
