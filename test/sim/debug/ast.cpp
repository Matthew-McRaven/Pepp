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

#include "sim/debug/expr_ast.hpp"

TEST_CASE("Evaluating const/volatile qualifiers on AST nodes", "[scope:debug][kind:unit][arch:*]") {
  using namespace pepp::debug;
  SECTION("Leaf nodes") {
    CHECK(Constant(int8_t(16)).cv_qualifiers() & CVQualifiers::Constant);
    CHECK(Variable("cat").cv_qualifiers() & CVQualifiers::Volatile);
  }
  SECTION("Internal nodes") {
    // Using nullptrs for inner, which should cause an exception if the nodes try to access their children.
    CHECK(UnaryPrefix(UnaryPrefix::Operators::ADDRESS_OF, nullptr).cv_qualifiers() & CVQualifiers::Volatile);
    CHECK(UnaryPrefix(UnaryPrefix::Operators::DEREFERENCE, nullptr).cv_qualifiers() & CVQualifiers::Volatile);
    CHECK(UnaryPrefix(UnaryPrefix::Operators::MINUS, nullptr).cv_qualifiers() == 0);
    CHECK(UnaryPrefix(UnaryPrefix::Operators::NEGATE, nullptr).cv_qualifiers() == 0);
    CHECK(BinaryInfix(BinaryInfix::Operators::STAR_DOT, nullptr, nullptr).cv_qualifiers() & CVQualifiers::Volatile);
    CHECK(BinaryInfix(BinaryInfix::Operators::DOT, nullptr, nullptr).cv_qualifiers() & CVQualifiers::Volatile);
    CHECK(BinaryInfix(BinaryInfix::Operators::ADD, nullptr, nullptr).cv_qualifiers() == 0);
    CHECK(BinaryInfix(BinaryInfix::Operators::GREATER, nullptr, nullptr).cv_qualifiers() == 0);
    CHECK(Parenthesized(nullptr).cv_qualifiers() == 0);
  }
}
