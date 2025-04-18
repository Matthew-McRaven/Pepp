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

TEST_CASE("Evaluating watch expressions", "[scope:debug][kind:unit][arch:*]") {
  using namespace pepp::debug;
  ZeroEnvironment env;
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
    CHECK(ast->evaluate(CachePolicy::UseAlways, env).type == ExpressionType::i16);
    CHECK(ast->evaluate(CachePolicy::UseAlways, env).bits == 13);
  }
  SECTION("Math with u8 and i16 (direct construction)") {
    ExpressionCache c;
    auto i16 = TypedBits{.allows_address_of = false, .type = ExpressionType::i16, .bits = 257};
    auto u8 = TypedBits{.allows_address_of = false, .type = ExpressionType::u8, .bits = 255};
    auto lhs = c.add_or_return(Constant(i16));
    auto rhs = c.add_or_return(Constant(u8));
    auto plus = c.add_or_return(BinaryInfix(BinaryInfix::Operators::ADD, lhs, rhs));
    auto eval = plus->evaluate(CachePolicy::UseAlways, env);
    CHECK(eval.bits == (257 + 255));
    CHECK(eval.type == ExpressionType::i16);
    CHECK(rhs->evaluate(CachePolicy::UseAlways, env).type == ExpressionType::u8);
  }
  SECTION("Parsing Math with u8 and i16 (parsing)") {
    ExpressionCache c;
    Parser p(c);
    QString body = "257_i16 + 255_u8";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto as_infix = std::dynamic_pointer_cast<BinaryInfix>(ast);
    REQUIRE(as_infix != nullptr);
    auto eval = as_infix->evaluate(CachePolicy::UseAlways, env);
    CHECK(eval.bits == (257 + 255));
    CHECK(eval.type == ExpressionType::i16);
    CHECK(as_infix->lhs->evaluate(CachePolicy::UseAlways, env).type == ExpressionType::i16);
    CHECK(as_infix->rhs->evaluate(CachePolicy::UseAlways, env).type == ExpressionType::u8);
  }

  SECTION("Parsing with explicit casts") {
    ExpressionCache c;
    Parser p(c);
    QString body = "(i8)(258 + 255)";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto as_cast = std::dynamic_pointer_cast<ExplicitCast>(ast);
    REQUIRE(as_cast != nullptr);
    auto as_paren = std::dynamic_pointer_cast<Parenthesized>(as_cast->arg);
    REQUIRE(as_paren != nullptr);
    auto as_infix = std::dynamic_pointer_cast<BinaryInfix>(as_paren->term);
    REQUIRE(as_infix != nullptr);
    auto eval = as_infix->evaluate(CachePolicy::UseAlways, env);
    CHECK(eval.bits == (258 + 255));
    CHECK(eval.type == ExpressionType::i16);
    CHECK(as_infix->lhs->evaluate(CachePolicy::UseAlways, env).type == ExpressionType::i16);
    CHECK(as_infix->rhs->evaluate(CachePolicy::UseAlways, env).type == ExpressionType::i16);

    // Overflows i8, so we should wrap-around.
    auto eval_casted = as_cast->evaluate(CachePolicy::UseAlways, env);
    CHECK(eval_casted.bits == (int8_t)((258 + 255) % 256));
    CHECK(eval_casted.type == ExpressionType::i8);
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

    ast->evaluate(CachePolicy::UseAlways, env);
    CHECK(top_plus->dirty() == false);
    CHECK(negb->dirty() == false);
    CHECK(b->dirty() == false);

    // mark_dirty only impacts current node
    b->mark_dirty();
    CHECK(b->dirty() == true);
    CHECK(negb->dirty() == false);
    CHECK(top_plus->dirty() == false);
    ast->evaluate(CachePolicy::UseNever, env);
    CHECK(b->dirty() == false);

    // But this helper marks all parents as dirty, without impacting siblings
    mark_parents_dirty(*b);
    CHECK(b->dirty() == true);
    CHECK(negb->dirty() == true);
    CHECK(top_plus->dirty() == true);
    CHECK(x->dirty() == false);
    CHECK(m->dirty() == false);
    CHECK(mx->dirty() == false);
    ast->evaluate(CachePolicy::UseNever, env);
    CHECK(top_plus->dirty() == false);
    CHECK(negb->dirty() == false);
    CHECK(b->dirty() == false);
  }
}

namespace {
struct MemoryArrayEnvironment : public pepp::debug::ZeroEnvironment {
  std::vector<uint8_t> mem = std::vector<uint8_t>(0x1'00'00, 7);
  inline uint8_t read_mem_u8(uint32_t addr) const override { return mem[addr % 0x1'00'00]; }
  inline uint16_t read_mem_u16(uint32_t addr) const override {
    return (uint16_t)(mem[addr % 0x1'00'00] << 8 | mem[(addr + 1) % 0x1'00'00]);
  }
};
} // namespace

TEST_CASE("Evaluations with environment access", "[scope:debug][kind:unit][arch:*]") {
  using namespace pepp::debug;
  MemoryArrayEnvironment env;

  SECTION("Memory Access") {
    ExpressionCache c;
    Parser p(c);
    QString body = "*0";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    CHECK(ast->evaluate(CachePolicy::UseNonVolatiles, env).bits == 0x0707);
    env.mem[0] = 8;
    // Ignoring requirement from volatiles to re-compute.
    CHECK(ast->evaluate(CachePolicy::UseAlways, env).bits == 0x0707);
    CHECK(ast->evaluate(CachePolicy::UseNonVolatiles, env).bits == 0x0807);
  }
  SECTION("Debugger Variables") {
    ExpressionCache c;
    Parser p(c);
    QString body = "$x";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    CHECK(ast->evaluate(CachePolicy::UseNonVolatiles, env).bits == 0);
  }
}
