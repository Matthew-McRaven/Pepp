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

#include "sim/debug/expr_parser.hpp"
#include "sim/debug/expr_tokenizer.hpp"

TEST_CASE("Parsing watch expressions", "[scope:sim][kind:unit][arch:*]") {
  using namespace pepp::debug;
  SECTION("Hex constants") {
    ExpressionCache c;
    Parser p(c);
    QString body = "0x15";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto as_const = std::dynamic_pointer_cast<Constant>(ast);
    REQUIRE(as_const != nullptr);
    CHECK(as_const->_val.format == detail::UnsignedConstant::Format::Hex);
    CHECK(as_const->_val.value == 0x15);
    CHECK(as_const->to_string() == "0x15");
  }
  SECTION("Unsigned Decimal constants") {
    ExpressionCache c;
    Parser p(c);
    QString body = "0115";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto as_const = std::dynamic_pointer_cast<Constant>(ast);
    REQUIRE(as_const != nullptr);
    CHECK(as_const->_val.format == detail::UnsignedConstant::Format::Dec);
    CHECK(as_const->_val.value == 115);
    CHECK(as_const->to_string() == "115");
  }
  SECTION("Identifier") {
    ExpressionCache c;
    Parser p(c);
    QString body = "limit";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto as_ident = std::dynamic_pointer_cast<Variable>(ast);
    REQUIRE(as_ident != nullptr);
    CHECK(as_ident->to_string() == "limit");
  }
  SECTION("Member Access with .") {
    ExpressionCache c;
    Parser p(c);
    QString body = "s.a";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto as_infix = std::dynamic_pointer_cast<BinaryInfix>(ast);
    REQUIRE(as_infix != nullptr);
    CHECK(as_infix->_op == BinaryInfix::Operators::DOT);
    CHECK(as_infix->to_string().toStdString() == "s.a");
  }
  SECTION("Member Access with ->") {
    ExpressionCache c;
    Parser p(c);
    QString body = "s->a";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto as_infix = std::dynamic_pointer_cast<BinaryInfix>(ast);
    REQUIRE(as_infix != nullptr);
    CHECK(as_infix->_op == BinaryInfix::Operators::STAR_DOT);
    CHECK(as_infix->to_string().toStdString() == "s->a");
  }
}
