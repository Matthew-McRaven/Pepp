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

TEST_CASE("Parsing watch expressions", "[scope:debug][kind:unit][arch:*]") {
  using namespace pepp::debug;
  SECTION("Hex constants") {
    ExpressionCache c;
    Parser p(c);
    QString body = "0x15";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto as_const = std::dynamic_pointer_cast<Constant>(ast);
    REQUIRE(as_const != nullptr);
    CHECK(as_const->_format_hint == detail::UnsignedConstant::Format::Hex);
    CHECK(as_const->_value.bits == 0x15);
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
    CHECK(as_const->_format_hint == detail::UnsignedConstant::Format::Dec);
    CHECK(as_const->_value.bits == 115);
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
  SECTION("(Unsigned decimal constants)") {
    ExpressionCache c;
    Parser p(c);
    QString body = "(0115)";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto as_par = std::dynamic_pointer_cast<Parenthesized>(ast);
    REQUIRE(as_par != nullptr);
    auto as_const = std::dynamic_pointer_cast<Constant>(as_par->_term);
    REQUIRE(as_const != nullptr);
    CHECK(as_const->_format_hint == detail::UnsignedConstant::Format::Dec);
    CHECK(as_const->_value.bits == 115);
    CHECK(as_const->to_string() == "115");
  }

  // P0
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
  // P1
  SECTION("Unary Prefix: +") {
    ExpressionCache c;
    Parser p(c);
    QString body = "+a";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto as_prefix = std::dynamic_pointer_cast<UnaryPrefix>(ast);
    REQUIRE(as_prefix != nullptr);
    CHECK(as_prefix->_op == UnaryPrefix::Operators::PLUS);
    CHECK(as_prefix->to_string().toStdString() == "+a");
  }
  SECTION("Unary Prefix: -") {
    ExpressionCache c;
    Parser p(c);
    QString body = "-a";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto as_prefix = std::dynamic_pointer_cast<UnaryPrefix>(ast);
    REQUIRE(as_prefix != nullptr);
    CHECK(as_prefix->_op == UnaryPrefix::Operators::MINUS);
    CHECK(as_prefix->to_string().toStdString() == "-a");
  }
  SECTION("Unary Prefix: *") {
    ExpressionCache c;
    Parser p(c);
    QString body = "*a";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto as_prefix = std::dynamic_pointer_cast<UnaryPrefix>(ast);
    REQUIRE(as_prefix != nullptr);
    CHECK(as_prefix->_op == UnaryPrefix::Operators::DEREFERENCE);
    CHECK(as_prefix->to_string().toStdString() == "*a");
  }
  SECTION("Unary Prefix: &") {
    ExpressionCache c;
    Parser p(c);
    QString body = "&a";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto as_prefix = std::dynamic_pointer_cast<UnaryPrefix>(ast);
    REQUIRE(as_prefix != nullptr);
    CHECK(as_prefix->_op == UnaryPrefix::Operators::ADDRESS_OF);
    CHECK(as_prefix->to_string().toStdString() == "&a");
  }
  SECTION("Unary Prefix: !") {
    ExpressionCache c;
    Parser p(c);
    QString body = "!a";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto as_prefix = std::dynamic_pointer_cast<UnaryPrefix>(ast);
    REQUIRE(as_prefix != nullptr);
    CHECK(as_prefix->_op == UnaryPrefix::Operators::NOT);
    CHECK(as_prefix->to_string().toStdString() == "!a");
  }
  SECTION("Unary Prefix: ~") {
    ExpressionCache c;
    Parser p(c);
    QString body = "~a";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto as_prefix = std::dynamic_pointer_cast<UnaryPrefix>(ast);
    REQUIRE(as_prefix != nullptr);
    CHECK(as_prefix->_op == UnaryPrefix::Operators::NEGATE);
    CHECK(as_prefix->to_string().toStdString() == "~a");
  }
  // P2
  SECTION("Multiply") {
    ExpressionCache c;
    Parser p(c);
    QString body = "s * 10";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto as_infix = std::dynamic_pointer_cast<BinaryInfix>(ast);
    REQUIRE(as_infix != nullptr);
    CHECK(as_infix->_op == BinaryInfix::Operators::MULTIPLY);
    CHECK(as_infix->to_string().toStdString() == "s * 10");
  }
  // P3
  SECTION("Add") {
    ExpressionCache c;
    Parser p(c);
    // Implicitly checks that numbers different numbers do not compare equal in expression cache
    QString body = "8 + 10";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto as_infix = std::dynamic_pointer_cast<BinaryInfix>(ast);
    REQUIRE(as_infix != nullptr);
    CHECK(as_infix->_op == BinaryInfix::Operators::ADD);
    CHECK(as_infix->to_string().toStdString() == "8 + 10");
  }
  SECTION("Mul-Add") {
    ExpressionCache c;
    Parser p(c);
    QString body = "5 * s + 6";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto as_infix = std::dynamic_pointer_cast<BinaryInfix>(ast);
    REQUIRE(as_infix != nullptr);
    CHECK(as_infix->_op == BinaryInfix::Operators::ADD);
    CHECK(as_infix->to_string().toStdString() == "5 * s + 6");
    auto as_nested_infix = std::dynamic_pointer_cast<BinaryInfix>(as_infix->_arg1);
    REQUIRE(as_nested_infix != nullptr);
    CHECK(as_nested_infix->_op == BinaryInfix::Operators::MULTIPLY);
    CHECK(as_nested_infix->to_string().toStdString() == "5 * s");
  }
  // P4
  SECTION("Shift Left") {
    ExpressionCache c;
    Parser p(c);
    // Implicitly checks that numbers different numbers do not compare equal in expression cache
    QString body = "8 << 10";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto as_infix = std::dynamic_pointer_cast<BinaryInfix>(ast);
    REQUIRE(as_infix != nullptr);
    CHECK(as_infix->_op == BinaryInfix::Operators::SHIFT_LEFT);
  }
  // P5
  SECTION("Inequality") {
    ExpressionCache c;
    Parser p(c);
    // Implicitly checks that numbers different numbers do not compare equal in expression cache
    QString body = "8 >= 10";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto as_infix = std::dynamic_pointer_cast<BinaryInfix>(ast);
    REQUIRE(as_infix != nullptr);
    CHECK(as_infix->_op == BinaryInfix::Operators::GREATER_OR_EQUAL);
    CHECK(as_infix->to_string().toStdString() == "8 >= 10");
  }
  // P6
  SECTION("Equality") {
    ExpressionCache c;
    Parser p(c);
    // Implicitly checks that numbers different numbers do not compare equal in expression cache
    QString body = "8 == 10";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto as_infix = std::dynamic_pointer_cast<BinaryInfix>(ast);
    REQUIRE(as_infix != nullptr);
    CHECK(as_infix->_op == BinaryInfix::Operators::EQUAL);
    CHECK(as_infix->to_string().toStdString() == "8 == 10");
  }
  // P7
  SECTION("Bitwise &") {
    ExpressionCache c;
    Parser p(c);
    // Implicitly checks that numbers different numbers do not compare equal in expression cache
    QString body = "8 & 10";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto as_infix = std::dynamic_pointer_cast<BinaryInfix>(ast);
    REQUIRE(as_infix != nullptr);
    CHECK(as_infix->_op == BinaryInfix::Operators::BIT_AND);
    CHECK(as_infix->to_string().toStdString() == "8 & 10");
  }
  // Tricky nested expressions
  SECTION("Mul-deref") {
    ExpressionCache c;
    Parser p(c);
    QString body = "5 * *s";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto as_infix = std::dynamic_pointer_cast<BinaryInfix>(ast);
    REQUIRE(as_infix != nullptr);
    CHECK(as_infix->_op == BinaryInfix::Operators::MULTIPLY);
    CHECK(as_infix->to_string().toStdString() == "5 * *s");
  }
  SECTION("Expressions at same level") {
    ExpressionCache c;
    Parser p(c);
    QString body = "5 * s * 2";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto as_infix = std::dynamic_pointer_cast<BinaryInfix>(ast);
    REQUIRE(as_infix != nullptr);
    CHECK(as_infix->_op == BinaryInfix::Operators::MULTIPLY);
    CHECK(as_infix->to_string().toStdString() == "5 * s * 2");
  }
  SECTION("Expressions at same level 2") {
    ExpressionCache c;
    Parser p(c);
    // Does not work if RHS recursion is at the wrong precendence level.
    QString body = "5 * 3 + 3 * 2";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto as_infix = std::dynamic_pointer_cast<BinaryInfix>(ast);
    REQUIRE(as_infix != nullptr);
    CHECK(as_infix->_op == BinaryInfix::Operators::ADD);
    CHECK(as_infix->to_string().toStdString() == "5 * 3 + 3 * 2");
  }
  SECTION("Now with ()") {
    ExpressionCache c;
    Parser p(c);
    // Does not work if RHS recursion is at the wrong precendence level.
    QString body = "5 * (3 + 3) * 2";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto as_infix = std::dynamic_pointer_cast<BinaryInfix>(ast);
    REQUIRE(as_infix != nullptr);
    CHECK(as_infix->_op == BinaryInfix::Operators::MULTIPLY);
    CHECK(as_infix->to_string().toStdString() == "5 * (3 + 3) * 2");
  }
}
