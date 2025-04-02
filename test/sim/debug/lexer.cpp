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

TEST_CASE("Lexing watch expressions", "[scope:sim][kind:unit][arch:*]") {
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
