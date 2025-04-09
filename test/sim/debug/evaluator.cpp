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

#include "sim/debug/expr_ast_ops.hpp"
#include "sim/debug/expr_parser.hpp"
#include "sim/debug/expr_tokenizer.hpp"

TEST_CASE("Evaluating watch expressions", "[scope:debug][kind:unit][arch:*]") {
  using namespace pepp::debug;
  SECTION("Expressions caching between compliations") {
    ExpressionCache c;
    Parser p(c);
    QString body = "m * x + -b";
    auto ast1 = p.compile(body);
    auto ast2 = p.compile(body);
    REQUIRE(ast1 != nullptr);
    CHECK((void *)ast1.get() == (void *)ast2.get());
  }
  SECTION("Dependency tracking") {
    ExpressionCache c;
    Parser p(c);
    QString body = "m * x + -b";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto top_plus = std::dynamic_pointer_cast<BinaryInfix>(ast);
    REQUIRE(top_plus->op == BinaryInfix::Operators::ADD);
    auto mx = std::dynamic_pointer_cast<BinaryInfix>(top_plus->lhs);
    REQUIRE(mx != nullptr);
    CHECK(mx->op == BinaryInfix::Operators::MULTIPLY);
    auto m = std::dynamic_pointer_cast<Variable>(mx->lhs);
    auto x = std::dynamic_pointer_cast<Variable>(mx->rhs);
    REQUIRE((m != nullptr && x != nullptr));

    auto negb = std::dynamic_pointer_cast<UnaryPrefix>(top_plus->rhs);
    REQUIRE(negb != nullptr);
    auto b = std::dynamic_pointer_cast<Variable>(negb->arg);
    REQUIRE(b != nullptr);

    // All direct dependencies
    CHECK(mx->dependency_of(top_plus));
    CHECK(m->dependency_of(mx));
    CHECK(x->dependency_of(mx));
    CHECK(negb->dependency_of(top_plus));
    CHECK(b->dependency_of(negb));
    // dependency_of does not commute
    CHECK(!top_plus->dependency_of(mx));
    CHECK(!mx->dependency_of(m));
    CHECK(!mx->dependency_of(x));
    CHECK(!top_plus->dependency_of(negb));
    CHECK(!negb->dependency_of(b));
    // dependency_of is not transitive
    CHECK(!top_plus->dependency_of(m));
    CHECK(!top_plus->dependency_of(x));
    CHECK(!top_plus->dependency_of(negb));
    CHECK(!top_plus->dependency_of(b));
  }
  SECTION("Evaluation of constants") {
    ExpressionCache c;
    Parser p(c);
    QString body = "3 * 3 + 4";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    CHECK(ast->evaluate(EvaluationMode::UseCache).type == ExpressionType::i16);
    CHECK(ast->evaluate(EvaluationMode::UseCache).bits == 13);
  }
  SECTION("Math with u8 and i16") {
    ExpressionCache c;
    auto i16 = TypedBits{.allows_address_of = false, .type = ExpressionType::i16, .bits = 257};
    auto u8 = TypedBits{.allows_address_of = false, .type = ExpressionType::u8, .bits = 255};
    auto lhs = c.add_or_return(Constant(i16));
    auto rhs = c.add_or_return(Constant(u8));
    auto plus = c.add_or_return(BinaryInfix(BinaryInfix::Operators::ADD, lhs, rhs));
    auto eval = plus->evaluate(EvaluationMode::UseCache);
    CHECK(eval.bits == (257 + 255));
    CHECK(eval.type == ExpressionType::i16);
    CHECK(rhs->evaluate(EvaluationMode::UseCache).type == ExpressionType::u8);
  }
  SECTION("Recursive dirtying") {
    ExpressionCache c;
    Parser p(c);
    QString body = "m * x + -b";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto top_plus = std::dynamic_pointer_cast<BinaryInfix>(ast);
    REQUIRE(top_plus->op == BinaryInfix::Operators::ADD);
    auto mx = std::dynamic_pointer_cast<BinaryInfix>(top_plus->lhs);
    REQUIRE(mx != nullptr);
    CHECK(mx->op == BinaryInfix::Operators::MULTIPLY);
    auto m = std::dynamic_pointer_cast<Variable>(mx->lhs);
    auto x = std::dynamic_pointer_cast<Variable>(mx->rhs);
    REQUIRE((m != nullptr && x != nullptr));

    auto negb = std::dynamic_pointer_cast<UnaryPrefix>(top_plus->rhs);
    REQUIRE(negb != nullptr);
    auto b = std::dynamic_pointer_cast<Variable>(negb->arg);
    REQUIRE(b != nullptr);

    ast->evaluate(EvaluationMode::UseCache);
    CHECK(top_plus->dirty() == false);
    CHECK(negb->dirty() == false);
    CHECK(b->dirty() == false);

    // mark_dirty only impacts current node
    b->mark_dirty();
    CHECK(b->dirty() == true);
    CHECK(negb->dirty() == false);
    CHECK(top_plus->dirty() == false);
    ast->evaluate(EvaluationMode::RecomputeTree);
    CHECK(b->dirty() == false);

    // But this helper marks all parents as dirty, without impacting siblings
    mark_parents_dirty(*b);
    CHECK(b->dirty() == true);
    CHECK(negb->dirty() == true);
    CHECK(top_plus->dirty() == true);
    CHECK(x->dirty() == false);
    CHECK(m->dirty() == false);
    CHECK(mx->dirty() == false);
    ast->evaluate(EvaluationMode::RecomputeTree);
    CHECK(top_plus->dirty() == false);
    CHECK(negb->dirty() == false);
    CHECK(b->dirty() == false);
  }
}
