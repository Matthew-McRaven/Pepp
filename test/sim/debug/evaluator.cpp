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
  using P = types::Primitives;
  SECTION("Expressions caching between compliations") {
    ExpressionCache c;
    ZeroEnvironment env;
    Parser p(c, *env.type_info());
    QString body = "m * x + -b";
    auto ast1 = p.compile(body);
    auto ast2 = p.compile(body);
    REQUIRE(ast1 != nullptr);
    CHECK((void *)ast1.get() == (void *)ast2.get());
  }
  SECTION("Dependency tracking") {
    ExpressionCache c;
    types::RuntimeTypeInfo t;
    types::NamedTypeInfo nti{t};
    Parser p(c, nti);
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
    ZeroEnvironment env;
    Parser p(c, *env.type_info());
    QString body = "3 * 3 + 4";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto ev = ast->evaluator();
    auto expected_type = types::Primitive{P::i16};
    CHECK(ev.cache().version == 0); // Though a constant expression, computation still performed and cache will update.
    CHECK(operators::op1_typeof(env.type_info()->info(), ev.evaluate(CachePolicy::UseAlways, env)) == expected_type);
    CHECK(value_bits(ev.evaluate(CachePolicy::UseAlways, env)) == 13);
    CHECK(ev.cache().version == 1);
  }
  SECTION("Math with u8 and i16 (direct construction)") {
    ExpressionCache c;
    ZeroEnvironment env;
    auto i16 = VPrimitive::from_int((int16_t)257);
    auto u8 = VPrimitive::from_int((uint8_t)255);
    auto lhs = c.add_or_return(Constant(i16));
    auto rhs = c.add_or_return(Constant(u8));
    auto plus = c.add_or_return(BinaryInfix(BinaryInfix::Operators::ADD, lhs, rhs));
    auto plus_ev = plus->evaluator(), rhs_eval = rhs->evaluator();
    CHECK(plus_ev.cache().version == 0);
    auto eval = plus_ev.evaluate(CachePolicy::UseAlways, env);
    CHECK(plus_ev.cache().version == 1);
    CHECK(value_bits(eval) == (257 + 255));
    CHECK(operators::op1_typeof(env.type_info()->info(), eval) == types::Primitive{P::i16});
    CHECK(rhs_eval.cache().version == 0); // Version of constant never changes.
    CHECK(operators::op1_typeof(env.type_info()->info(), rhs_eval.evaluate(CachePolicy::UseAlways, env)) ==
          types::Primitive{P::u8});
    CHECK(rhs_eval.cache().version == 0);
  }
  SECTION("Parsing Math with u8 and i16 (parsing)") {
    ExpressionCache c;
    ZeroEnvironment env;
    Parser p(c, *env.type_info());
    QString body = "257_i16 + 255_u8";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto as_infix = std::dynamic_pointer_cast<BinaryInfix>(ast);
    REQUIRE(as_infix != nullptr);
    auto as_infix_eval = as_infix->evaluator();
    auto eval = as_infix_eval.evaluate(CachePolicy::UseAlways, env);
    CHECK(value_bits(eval) == (257 + 255));
    CHECK(operators::op1_typeof(env.type_info()->info(), eval) == types::Primitive{P::i16});
    auto lhs_eval = as_infix->lhs->evaluator();
    auto rhs_eval = as_infix->rhs->evaluator();
    CHECK(operators::op1_typeof(env.type_info()->info(), lhs_eval.evaluate(CachePolicy::UseAlways, env)) ==
          types::Primitive{P::i16});
    CHECK(operators::op1_typeof(env.type_info()->info(), rhs_eval.evaluate(CachePolicy::UseAlways, env)) ==
          types::Primitive{P::u8});
  }
  SECTION("Parsing with direct casts") {
    ExpressionCache c;
    ZeroEnvironment env;
    Parser p(c, *env.type_info());
    QString body = "(i8)(258 + 255)";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto as_cast = std::dynamic_pointer_cast<DirectCast>(ast);
    REQUIRE(as_cast != nullptr);
    auto as_paren = std::dynamic_pointer_cast<Parenthesized>(as_cast->arg);
    REQUIRE(as_paren != nullptr);
    auto as_infix = std::dynamic_pointer_cast<BinaryInfix>(as_paren->term);
    REQUIRE(as_infix != nullptr);
    auto as_infix_eval = as_infix->evaluator();
    auto eval = as_infix_eval.evaluate(CachePolicy::UseAlways, env);
    CHECK(value_bits(eval) == (258 + 255));
    CHECK(operators::op1_typeof(env.type_info()->info(), eval) == types::Primitive{P::i16});
    auto lhs_eval = as_infix->lhs->evaluator();
    auto rhs_eval = as_infix->rhs->evaluator();
    CHECK(operators::op1_typeof(env.type_info()->info(), lhs_eval.evaluate(CachePolicy::UseAlways, env)) ==
          types::Primitive{P::i16});
    CHECK(operators::op1_typeof(env.type_info()->info(), rhs_eval.evaluate(CachePolicy::UseAlways, env)) ==
          types::Primitive{P::i16});

    // Overflows i8, so we should wrap-around.
    auto as_cast_eval = as_cast->evaluator();
    auto eval_casted = as_cast_eval.evaluate(CachePolicy::UseAlways, env);
    CHECK(value_bits(eval_casted) == (int8_t)((258 + 255) % 256));
    CHECK(operators::op1_typeof(env.type_info()->info(), eval_casted) == types::Primitive{P::i8});
  }
  SECTION("Evaluation with indirect cast") {
    ExpressionCache c;
    ZeroEnvironment env;
    auto &nti = *env.type_info();
    auto value = c.add_or_return(Constant{VPrimitive::from_int((uint16_t)257)});
    auto myint_hnd = env.type_info()->register_name("my_int");
    auto cast = c.add_or_return(IndirectCast("my_int", myint_hnd.second, value));
    auto eval_value = value->evaluator();
    auto res_value = eval_value.evaluate(CachePolicy::UseNonVolatiles, env);
    CHECK(value_bits(res_value) == 257);
    CHECK(operators::op1_typeof(env.type_info()->info(), res_value) == types::Primitive{P::u16});
    auto eval_cast = cast->evaluator();
    // Type has been declared but not defined. Should evaluate to never/undefined
    auto res_cast = eval_cast.evaluate(CachePolicy::UseNonVolatiles, env);
    CHECK(res_cast == VNever{});
    // As u8, should invalidate cache
    nti.set_type(myint_hnd.second, env.type_info()->info().box(types::Primitive{types::Primitives::u8}));
    res_cast = eval_cast.evaluate(CachePolicy::UseNonVolatiles, env);
    CHECK(operators::op1_typeof(env.type_info()->info(), res_cast) == types::Primitive{P::u8});
    CHECK(value_bits(res_cast) == (uint8_t)(257 % 256));
    // As i16, should invalidate cache
    nti.set_type(myint_hnd.second, env.type_info()->info().box(types::Primitive{types::Primitives::i16}));
    res_cast = eval_cast.evaluate(CachePolicy::UseNonVolatiles, env);
    CHECK(operators::op1_typeof(env.type_info()->info(), res_cast) == types::Primitive{P::i16});
    CHECK(value_bits(res_cast) == 257);
  }

  SECTION("Recursive dirtying") {
    ExpressionCache c;
    ZeroEnvironment env;
    Parser p(c, *env.type_info());
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
    auto ast_eval = ast->evaluator();
    ast_eval.evaluate(CachePolicy::UseAlways, env);
    CHECK(top_plus->dirty() == false);
    CHECK(negb->dirty() == false);
    CHECK(b->dirty() == false);

    // mark_dirty only impacts current node
    b->mark_dirty();
    CHECK(b->dirty() == true);
    CHECK(negb->dirty() == false);
    CHECK(top_plus->dirty() == false);
    ast_eval.evaluate(CachePolicy::UseNever, env);
    CHECK(b->dirty() == false);

    // But this helper marks all parents as dirty, without impacting siblings
    mark_parents_dirty(*b);
    CHECK(b->dirty() == true);
    CHECK(negb->dirty() == true);
    CHECK(top_plus->dirty() == true);
    CHECK(x->dirty() == false);
    CHECK(m->dirty() == false);
    CHECK(mx->dirty() == false);
    ast_eval.evaluate(CachePolicy::UseNever, env);
    CHECK(top_plus->dirty() == false);
    CHECK(negb->dirty() == false);
    CHECK(b->dirty() == false);

    // This time don't attempt to recompute anything!
    mark_parents_dirty(*b);
    CHECK(b->dirty() == true);
    CHECK(negb->dirty() == true);
    CHECK(top_plus->dirty() == true);
    CHECK(x->dirty() == false);
    CHECK(m->dirty() == false);
    CHECK(mx->dirty() == false);
    ast_eval.evaluate(CachePolicy::UseDirtyAlways, env);
    CHECK(top_plus->dirty() == true);
    CHECK(negb->dirty() == true);
    CHECK(b->dirty() == true);
    CHECK(m->dirty() == false);
    CHECK(mx->dirty() == false);
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
    Parser p(c, *env.type_info());
    QString body = "*0";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto ast_eval = ast->evaluator();
    CHECK(value_bits(ast_eval.evaluate(CachePolicy::UseNonVolatiles, env)) == 0x0707);
    env.mem[0] = 8;
    // Ignoring requirement from volatiles to re-compute.
    CHECK(value_bits(ast_eval.evaluate(CachePolicy::UseAlways, env)) == 0x0707);
    CHECK(value_bits(ast_eval.evaluate(CachePolicy::UseNonVolatiles, env)) == 0x0807);
  }
  SECTION("Debugger Variables") {
    ExpressionCache c;
    Parser p(c, *env.type_info());
    QString body = "$x";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto ast_eval = ast->evaluator();
    CHECK(value_bits(ast_eval.evaluate(CachePolicy::UseNonVolatiles, env)) == 0);
  }
}
