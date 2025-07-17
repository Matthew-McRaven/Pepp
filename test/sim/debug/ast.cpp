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
#include "sim/debug/expr_ast_ops.hpp"
#include "sim/debug/expr_parser.hpp"

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
    CHECK(MemberAccess(BinaryInfix::Operators::STAR_DOT, nullptr, "").cv_qualifiers() & CVQualifiers::Volatile);
    CHECK(MemberAccess(BinaryInfix::Operators::DOT, nullptr, "").cv_qualifiers() & CVQualifiers::Volatile);
    CHECK(BinaryInfix(BinaryInfix::Operators::ADD, nullptr, nullptr).cv_qualifiers() == 0);
    CHECK(BinaryInfix(BinaryInfix::Operators::GREATER, nullptr, nullptr).cv_qualifiers() == 0);
    CHECK(Parenthesized(nullptr).cv_qualifiers() == 0);
  }
}

struct CountEvalVisitor : public pepp::debug::ConstantTermVisitor {
  std::set<const pepp::debug::Term *> visited;
  void accept(const pepp::debug::Variable &node) override { visited.insert(&node); }
  void accept(const pepp::debug::DebuggerVariable &node) override { visited.insert(&node); }
  void accept(const pepp::debug::Constant &node) override { visited.insert(&node); }
  void accept(const pepp::debug::BinaryInfix &node) override {
    visited.insert(&node);
    node.lhs->accept(*this);
    node.rhs->accept(*this);
  }
  void accept(const pepp::debug::MemberAccess &node) override {
    visited.insert(&node);
    node.lhs->accept(*this);
  }
  void accept(const pepp::debug::UnaryPrefix &node) override {
    visited.insert(&node);
    node.arg->accept(*this);
  }
  void accept(const pepp::debug::MemoryRead &node) override {
    visited.insert(&node);
    node.arg->accept(*this);
  }
  void accept(const pepp::debug::Parenthesized &node) override { node.term->accept(*this); }
  void accept(const pepp::debug::DirectCast &node) override {
    visited.insert(&node);
    node.arg->accept(*this);
  }
  void accept(const pepp::debug::IndirectCast &node) override {
    visited.insert(&node);
    node.arg->accept(*this);
  }
};

std::size_t term_count(std::shared_ptr<const pepp::debug::Term> root) {
  CountEvalVisitor v;
  root->accept(v);
  return v.visited.size();
}

TEST_CASE("Non-mutating visitor", "[scope:debug][kind:unit][arch:*]") {
  using namespace pepp::debug;
  ExpressionCache c;
  auto constant = c.add_or_return(Constant(int8_t(16)));
  auto variable = c.add_or_return(Variable("cat"));
  CHECK(term_count(constant) == 1);
  CHECK(term_count(variable) == 1);
  auto unary = c.add_or_return(UnaryPrefix(UnaryPrefix::Operators::MINUS, constant));
  CHECK(term_count(unary) == 2);
  auto infix = c.add_or_return(BinaryInfix(BinaryInfix::Operators::ADD, constant, variable));
  CHECK(term_count(infix) == 3);
  auto duplicated_infix = c.add_or_return(BinaryInfix(BinaryInfix::Operators::MODULO, unary, constant));
  CHECK(term_count(duplicated_infix) == 3);
}
TEST_CASE("Recursive CV visitors", "[scope:debug][kind:unit][arch:*]") {
  using namespace pepp::debug;

  SECTION("Recursively Constant Expressions") {
    ExpressionCache c;
    auto constant = c.add_or_return(Constant(int8_t(16)));
    auto variable = c.add_or_return(Variable("cat"));
    CHECK(is_constant_expression(*constant));
    CHECK(!is_constant_expression(*variable));
    CHECK(!is_constant_expression(UnaryPrefix(UnaryPrefix::Operators::ADDRESS_OF, constant)));
    CHECK(!is_constant_expression(UnaryPrefix(UnaryPrefix::Operators::DEREFERENCE, constant)));
    CHECK(is_constant_expression(UnaryPrefix(UnaryPrefix::Operators::MINUS, constant)));
    CHECK(!is_constant_expression(UnaryPrefix(UnaryPrefix::Operators::ADDRESS_OF, variable)));
    CHECK(!is_constant_expression(UnaryPrefix(UnaryPrefix::Operators::DEREFERENCE, variable)));
    CHECK(!is_constant_expression(UnaryPrefix(UnaryPrefix::Operators::MINUS, variable)));
    auto unary_constant = c.add_or_return(UnaryPrefix(UnaryPrefix::Operators::MINUS, constant));

    CHECK(!is_constant_expression(MemberAccess(BinaryInfix::Operators::STAR_DOT, constant, "cat")));
    CHECK(!is_constant_expression(MemberAccess(BinaryInfix::Operators::DOT, constant, "cat")));
    CHECK(is_constant_expression(BinaryInfix(BinaryInfix::Operators::ADD, constant, unary_constant)));
    CHECK(is_constant_expression(BinaryInfix(BinaryInfix::Operators::GREATER, constant, unary_constant)));
    CHECK(!is_constant_expression(MemberAccess(BinaryInfix::Operators::STAR_DOT, variable, "cat")));
    CHECK(!is_constant_expression(MemberAccess(BinaryInfix::Operators::DOT, variable, "cat")));
    CHECK(!is_constant_expression(BinaryInfix(BinaryInfix::Operators::ADD, variable, unary_constant)));
    CHECK(!is_constant_expression(BinaryInfix(BinaryInfix::Operators::GREATER, variable, unary_constant)));

    CHECK(is_constant_expression(Parenthesized(constant)));
    CHECK(!is_constant_expression(Parenthesized(variable)));
  }
  SECTION("Gather volatile terms") {
    ExpressionCache c;
    auto constant = c.add_or_return(Constant(int8_t(16)));
    auto variable = c.add_or_return(Variable("cat"));
    auto unary_constant = c.add_or_return(UnaryPrefix(UnaryPrefix::Operators::MINUS, constant));
    auto unary_volatile = c.add_or_return(UnaryPrefix(UnaryPrefix::Operators::ADDRESS_OF, unary_constant));
    auto binary_contains_v = c.add_or_return(BinaryInfix(BinaryInfix::Operators::ADD, unary_volatile, variable));
    auto binary_volatile = c.add_or_return(MemberAccess(BinaryInfix::Operators::STAR_DOT, constant, "cat"));
    auto top = c.add_or_return(BinaryInfix(BinaryInfix::Operators::ADD, binary_volatile, binary_contains_v));
    auto v = volatiles(*top);
    auto contains = [v](std::shared_ptr<Term> term) {
      auto it = std::find(v.cbegin(), v.cend(), term);
      if (it == v.cend()) return false;
      return true;
    };
    CHECK(v.size() == 3);
    CHECK(!contains(std::dynamic_pointer_cast<pepp::debug::Term>(constant)));
    CHECK(contains(std::dynamic_pointer_cast<pepp::debug::Term>(variable)));
    CHECK(!contains(std::dynamic_pointer_cast<pepp::debug::Term>(unary_constant)));
    CHECK(contains(std::dynamic_pointer_cast<pepp::debug::Term>(unary_volatile)));
    CHECK(!contains(std::dynamic_pointer_cast<pepp::debug::Term>(binary_contains_v)));
    CHECK(contains(std::dynamic_pointer_cast<pepp::debug::Term>(binary_volatile)));
    CHECK(!contains(std::dynamic_pointer_cast<pepp::debug::Term>(top)));
  }
}
