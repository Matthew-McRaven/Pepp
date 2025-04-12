/*
 * Copyright (c) 2025 J. Stanley Warford, Matthew McRaven
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

#include "sim/debug/expr_tokenizer.hpp"

TEST_CASE("Lexing watch expressions", "[scope:debug][kind:unit][arch:*]") {
  using namespace pepp::debug;
  SECTION("Decimal Constants") {
    QString body = "1024";
    Lexer l(body);
    auto token = l.next_token();
    REQUIRE(std::holds_alternative<detail::UnsignedConstant>(token));
    auto narrowed = std::get<detail::UnsignedConstant>(token);
    CHECK(narrowed.format == detail::UnsignedConstant::Format::Dec);
    CHECK(narrowed.value == 1024);
  }
  SECTION("Decimal Constant with trailing type") {
    QString body = "1024_u8";
    Lexer l(body);
    auto token = l.next_token();
    REQUIRE(std::holds_alternative<detail::UnsignedConstant>(token));
    auto narrowed = std::get<detail::UnsignedConstant>(token);
    CHECK(narrowed.format == detail::UnsignedConstant::Format::Dec);
    CHECK(narrowed.value == 1024);
    auto token2 = l.next_token();
    REQUIRE(std::holds_alternative<detail::TypeSuffix>(token2));
    auto narrowed2 = std::get<detail::TypeSuffix>(token2);
    CHECK(narrowed2.type == ExpressionType::u8);
  }
  SECTION("No free floating trailing types") {
    QString body = "_u8";
    Lexer l(body);
    auto token = l.next_token();
    REQUIRE(std::holds_alternative<detail::Identifier>(token));
  }
  SECTION("No spaces with trailing type") {
    QString body = "1024 _u8";
    Lexer l(body);
    auto token = l.next_token();
    REQUIRE(std::holds_alternative<detail::UnsignedConstant>(token));
    auto narrowed = std::get<detail::UnsignedConstant>(token);
    CHECK(narrowed.format == detail::UnsignedConstant::Format::Dec);
    CHECK(narrowed.value == 1024);
    auto token2 = l.next_token();
    REQUIRE(std::holds_alternative<detail::Identifier>(token2));
  }
  SECTION("Explicit type cast") {
    QString body = "(u8)";
    Lexer l(body);
    auto token = l.next_token();
    REQUIRE(std::holds_alternative<detail::TypeCast>(token));
    auto narrowed = std::get<detail::TypeCast>(token);
    CHECK(narrowed.type == ExpressionType::u8);
  }
  SECTION("Debug Identifier") {
    QString body = "$X$A$lala";
    Lexer l(body);
    {
      auto token = l.next_token();
      REQUIRE(std::holds_alternative<detail::DebugIdentifier>(token));
      auto narrowed = std::get<detail::DebugIdentifier>(token);
      CHECK(narrowed.value == "X");
    }
    {
      auto token = l.next_token();
      REQUIRE(std::holds_alternative<detail::DebugIdentifier>(token));
      auto narrowed = std::get<detail::DebugIdentifier>(token);
      CHECK(narrowed.value == "A");
    }
    {
      auto token = l.next_token();
      REQUIRE(std::holds_alternative<detail::DebugIdentifier>(token));
      auto narrowed = std::get<detail::DebugIdentifier>(token);
      CHECK(narrowed.value == "lala");
    }
  }

  SECTION("Whole Expressions") {
    QString body = "0x25+($x *1024 )%5 + hello->yeet";
    Lexer l(body);
    std::vector<Lexer::Token> tokens;
    for (Lexer::Token token(l.next_token()); token.index() >= 3; token = l.next_token()) tokens.emplace_back(token);
    REQUIRE(tokens.size() == 13);
    {
      REQUIRE(std::holds_alternative<detail::UnsignedConstant>(tokens[0]));
      auto token = std::get<detail::UnsignedConstant>(tokens[0]);
      CHECK(token.format == detail::UnsignedConstant::Format::Hex);
      CHECK(token.value == 0x25);
    }
    {
      REQUIRE(std::holds_alternative<detail::Literal>(tokens[1]));
      auto token = std::get<detail::Literal>(tokens[1]);
      CHECK(token.literal == "+");
    }
    {
      REQUIRE(std::holds_alternative<detail::Literal>(tokens[2]));
      auto token = std::get<detail::Literal>(tokens[2]);
      CHECK(token.literal == "(");
    }
    CHECK(std::holds_alternative<detail::DebugIdentifier>(tokens[3]));
    CHECK(std::holds_alternative<detail::Literal>(tokens[4]));
    {
      REQUIRE(std::holds_alternative<detail::UnsignedConstant>(tokens[5]));
      auto token = std::get<detail::UnsignedConstant>(tokens[5]);
      CHECK(token.format == detail::UnsignedConstant::Format::Dec);
      CHECK(token.value == 1024);
    }
    CHECK(std::holds_alternative<detail::Literal>(tokens[6]));
    CHECK(std::holds_alternative<detail::Literal>(tokens[7]));
    {
      REQUIRE(std::holds_alternative<detail::UnsignedConstant>(tokens[8]));
      auto token = std::get<detail::UnsignedConstant>(tokens[8]);
      CHECK(token.format == detail::UnsignedConstant::Format::Dec);
      CHECK(token.value == 5);
    }
    CHECK(std::holds_alternative<detail::Literal>(tokens[9]));
    CHECK(std::holds_alternative<detail::Identifier>(tokens[10]));
    CHECK(std::holds_alternative<detail::Literal>(tokens[11]));
    CHECK(std::holds_alternative<detail::Identifier>(tokens[12]));
  }
}
