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

namespace {
struct MemoryArrayEnvironment : public pepp::debug::ZeroEnvironment {
  mutable std::map<quint32, uint8_t> mem;
  mutable std::map<QString, pepp::debug::Value> variables;
  inline uint8_t read_mem_u8(uint32_t addr) const override {
    if (mem.contains(addr)) return mem[addr];
    return 0;
  }
  inline uint16_t read_mem_u16(uint32_t addr) const override {
    return (uint16_t)(read_mem_u8(addr) << 8 | read_mem_u8(addr + 1));
  }
  inline pepp::debug::Value evaluate_variable(QStringView name) const override {
    if (variables.contains(QString{name})) return variables[QString{name}];
    else return pepp::debug::VPrimitive::from_int(int16_t(0));
  };
};
} // namespace

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
    types::TypeInfo info;
    Parser p(c, info);
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
    CHECK(operators::op1_typeof(*env.type_info(), ev.evaluate(CachePolicy::UseAlways, env)) == expected_type);
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
    CHECK(operators::op1_typeof(*env.type_info(), eval) == types::Primitive{P::i16});
    CHECK(rhs_eval.cache().version == 0); // Version of constant never changes.
    CHECK(operators::op1_typeof(*env.type_info(), rhs_eval.evaluate(CachePolicy::UseAlways, env)) ==
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
    CHECK(operators::op1_typeof(*env.type_info(), eval) == types::Primitive{P::i16});
    auto lhs_eval = as_infix->lhs->evaluator();
    auto rhs_eval = as_infix->rhs->evaluator();
    CHECK(operators::op1_typeof(*env.type_info(), lhs_eval.evaluate(CachePolicy::UseAlways, env)) ==
          types::Primitive{P::i16});
    CHECK(operators::op1_typeof(*env.type_info(), rhs_eval.evaluate(CachePolicy::UseAlways, env)) ==
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
    CHECK(operators::op1_typeof(*env.type_info(), eval) == types::Primitive{P::i16});
    auto lhs_eval = as_infix->lhs->evaluator();
    auto rhs_eval = as_infix->rhs->evaluator();
    CHECK(operators::op1_typeof(*env.type_info(), lhs_eval.evaluate(CachePolicy::UseAlways, env)) ==
          types::Primitive{P::i16});
    CHECK(operators::op1_typeof(*env.type_info(), rhs_eval.evaluate(CachePolicy::UseAlways, env)) ==
          types::Primitive{P::i16});

    // Overflows i8, so we should wrap-around.
    auto as_cast_eval = as_cast->evaluator();
    auto eval_casted = as_cast_eval.evaluate(CachePolicy::UseAlways, env);
    CHECK(value_bits(eval_casted) == (int8_t)((258 + 255) % 256));
    CHECK(operators::op1_typeof(*env.type_info(), eval_casted) == types::Primitive{P::i8});
  }
  SECTION("Parsing with indirect cast") {
    ExpressionCache c;
    ZeroEnvironment env;
    auto &nti = *env.type_info();
    Parser p(c, nti);
    QString body = "(my_int*)257";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto as_cast = std::dynamic_pointer_cast<IndirectCast>(ast);
    REQUIRE(as_cast != nullptr);
    auto as_const = std::dynamic_pointer_cast<Constant>(as_cast->arg);
    REQUIRE(as_const != nullptr);

    // Leaf node has right type
    auto const_eval = as_const->evaluator();
    auto const_value = const_eval.evaluate(CachePolicy::UseNonVolatiles, env);
    CHECK(value_bits(const_value) == 257);
    CHECK(operators::op1_typeof(*env.type_info(), const_value) == types::Primitive{P::i16});

    // Parent node
    auto cast_eval = ast->evaluator();
    // Type has been declared but not defined. Should evaluate to never/undefined
    auto cast_value = cast_eval.evaluate(CachePolicy::UseNonVolatiles, env);
    auto hnd = nti.register_indirect("my_int");
    CHECK(cast_value == VNever{});
    // As u8, should invalidate cache
    nti.set_indirect_type(hnd.second, nti.register_direct(types::Primitives::u8));
    cast_value = cast_eval.evaluate(CachePolicy::UseNonVolatiles, env);
    CHECK(operators::op1_typeof(*env.type_info(), cast_value) == types::Primitive{P::u8});
    CHECK(value_bits(cast_value) == (uint8_t)(257 % 256));
    // As i16, should invalidate cache
    nti.set_indirect_type(hnd.second, nti.register_direct(types::Primitives::i16));
    cast_value = cast_eval.evaluate(CachePolicy::UseNonVolatiles, env);
    CHECK(operators::op1_typeof(*env.type_info(), cast_value) == types::Primitive{P::i16});
    CHECK(value_bits(cast_value) == 257);
  }
  SECTION("Parsing with member access") {
    ExpressionCache c;
    MemoryArrayEnvironment env;
    auto &nti = *env.type_info();
    Parser p(c, nti);
    auto boxed_i8 = nti.box(types::Primitives::i8);
    pepp::debug::types::Type i8p = types::Pointer{2, boxed_i8};
    auto boxed_i8p = nti.box(i8p);
    types::Struct mystruct;
    mystruct.members.emplace_back(types::Struct::Tuple{"m1", boxed_i8, 0});
    mystruct.members.emplace_back(types::Struct::Tuple{"b", boxed_i8, 2});
    pepp::debug::types::Type mystruct_type = mystruct;
    auto hnd_struct = nti.register_direct(mystruct_type);
    QString body = "a.b";
    auto ast = p.compile(body);
    REQUIRE(ast != nullptr);
    auto as_cast = std::dynamic_pointer_cast<MemberAccess>(ast);
    REQUIRE(as_cast != nullptr);
    CHECK(as_cast->rhs == "b");
    auto as_var = std::dynamic_pointer_cast<Variable>(as_cast->lhs);
    REQUIRE(as_var != nullptr);
    CHECK(as_var->name == "a");
    // No bound types means we get back never
    auto eval_cast = as_cast->evaluator();
    auto value_cast = eval_cast.evaluate(CachePolicy::UseNonVolatiles, env);
    CHECK(value_cast == VNever{});
    // Update variable type in environment. A has base address of 2, so a.b should touch only address 4.
    env.variables["a"] = pepp::debug::VStruct{hnd_struct, 0x02};
    // memory addresses from 2 to 7 will have non-0 values
    for (int it = 0; it < 5; it++) env.mem[it + 2] = it + 3;
    auto eval_var = as_var->evaluator();
    auto value_var = eval_var.evaluate(CachePolicy::UseNonVolatiles, env);
    CHECK(operators::op1_typeof(nti, value_var) == mystruct);
    value_cast = eval_cast.evaluate(CachePolicy::UseNonVolatiles, env);
    CHECK(operators::op1_typeof(nti, value_cast) == i8p);
    CHECK(value_bits(value_cast) == 0x4);
  }
  SECTION("Deref member access ") {
    ExpressionCache c;
    MemoryArrayEnvironment env;
    auto &nti = *env.type_info();
    Parser p(c, nti);
    auto i8 = types::Primitive{types::Primitives::i8};
    auto boxed_i8 = nti.box(types::Primitives::i8);
    types::Type wrapped_i8p = types::Pointer{2, boxed_i8};
    auto boxed_i8p = nti.box(wrapped_i8p);
    types::Struct mystruct;
    mystruct.members.emplace_back(types::Struct::Tuple{"b", boxed_i8, 2});
    pepp::debug::types::Type wrapped_struct = mystruct;
    auto hnd_struct = nti.register_direct(wrapped_struct);
    // Update variable type in environment. A has base address of 2, so a.b should touch only address 4.
    env.variables["a"] = pepp::debug::VStruct{hnd_struct, 0x02};
    auto ast = p.compile("*(a.b)");
    REQUIRE(ast != nullptr);
    // memory addresses from 2 to 7 will have non-0 values
    for (int it = 0; it < 5; it++) env.mem[it + 2] = it + 3;
    auto eval_ast = ast->evaluator();
    auto value_ast = eval_ast.evaluate(CachePolicy::UseNonVolatiles, env);
    CHECK(operators::op1_typeof(nti, value_ast) == i8);
    CHECK(value_bits(value_ast) == 0x5);
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


TEST_CASE("Evaluations with environment access", "[scope:debug][kind:unit][arch:*]") {
  using namespace pepp::debug;
  MemoryArrayEnvironment env;
  env.type_info()->box(types::Primitives::u16);
  env.type_info()->box(types::Primitives::i16);
  env.mem[0] = 7;
  env.mem[1] = 7;
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
