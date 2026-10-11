/*
 * Copyright (c) 2026. Stanley Warford, Matthew McRaven
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "core/langs/expr/evaluator.hpp"
#include <catch.hpp>
#include <array>
#include <limits>
#include <vector>
#include "core/langs/expr/parser.hpp"

namespace {
using namespace pepp::tc::expr;
using enum Signedness;

// One constexpr and one  non-constexpr function which preserve argument's types.
std::expected<Value, NullaryError> twice(Value x, Type) { return Value{x.bits * 2, x.type}; }
Type same(Type type, Type) { return type; }
constexpr std::array<Function, 2> functions{{{"%twice", twice, same}, {"%where", nullptr, same}}};

Tree tree_of(const char *source) {
  auto result = parse(source);
  REQUIRE(std::holds_alternative<Parsed>(result));
  return std::get<Parsed>(std::move(result)).tree;
}

std::expected<Value, EvaluationError> evaluate(const char *source, Type default_type) {
  return evaluate_expression(tree_of(source), default_type);
}
} // namespace

TEST_CASE("Expression evaluation", "[scope:core][scope:core.langs][kind:unit][arch:*]") {
  // Pep/10 has a 16-bit int and treats numbers as u16 by default.
  // RISC-V has a 32-bit int and is signed by default.
  const Type pep{16, Unsigned}, pep_signed{16, Signed}, rv{32, Signed};
  constexpr u64 max = std::numeric_limits<u64>::max();

  SECTION("Values and types") {
    struct Case {
      const char *source;
      Type default_type;
      Value value;
    };
    const std::vector<Case> cases = {
        // Literals are the smallest type which can hold the bit pattern (either signed or unsigned). Starts at
        // sizeof(int)
        {"42", pep, {42, {16, Bits}}},
        {"0xFFFF", pep, {0xFFFF, {16, Bits}}},
        {"65535", pep, {65535, {16, Bits}}},
        {"0x10000", pep, {0x10000, {32, Bits}}},
        {"0xFFFF", rv, {0xFFFF, {32, Bits}}},
        {"0xFFFFFFFFFFFFFFFF", pep, {max, {64, Bits}}},
        {"'a'", pep, {'a', {16, Bits}}},
        // Sign-agnostic operators preserve "bit" inputs
        {"0xFFFF + 1", pep, {0, {16, Bits}}},
        {"~0", pep, {0xFFFF, {16, Bits}}},
        {"1 << 15", pep, {0x8000, {16, Bits}}},
        // Negation forces signedness
        {"-1", pep, {0xFFFF, {16, Signed}}},
        {"-4 / 2", pep, {0xFFFE, {16, Signed}}},
        {"-16 >> 2", pep, {0xFFFC, {16, Signed}}},
        // Non-signed bits follow default
        {"0xFFFF / 2", pep, {0x7FFF, {16, Unsigned}}},
        {"0xFFFF / 2", pep_signed, {0, {16, Signed}}},
        {"0xFFFF >> 4", pep, {0x0FFF, {16, Unsigned}}},
        {"0xFFFF >> 4", pep_signed, {0xFFFF, {16, Signed}}},
        {"0x10000 + 0xFFFF", pep, {0x1FFFF, {32, Unsigned}}}, // Widening bits also needs a sign.
        // Logic and comparisons always result in an int that is 0 or 1.
        {"0xFFFF > -1", pep, {0, {16, Signed}}}, // As in C: 0xFFFF reads as -1 against a signed -1.
        {"0xFFFFFFFFFFFFFFFF > -1", pep, {0, {16, Signed}}},
        {"0xFFFF < 1", pep, {0, {16, Signed}}},
        {"0xFFFF < 1", pep_signed, {1, {16, Signed}}},
        {"3 == 3", pep, {1, {16, Signed}}},
        {"!5", pep, {0, {16, Signed}}},
        {"2 && 3", pep, {1, {16, Signed}}},
        {"0 && 1 / 0", pep, {0, {16, Signed}}}, // Short-circuits.
        {"1 || 1 / 0", pep, {1, {16, Signed}}},
    };
    for (const auto &c : cases) {
      CAPTURE(c.source, c.default_type.bits, static_cast<int>(c.default_type.sign));
      const auto result = evaluate(c.source, c.default_type);
      REQUIRE(result.has_value());
      CHECK(result->bits == c.value.bits);
      CHECK(result->type.bits == c.value.type.bits);
      CHECK(result->type.sign == c.value.type.sign);
    }
  }
  SECTION("Errors") {
    struct Case {
      const char *source;
      ErrorCode code;
      const char *argument = "";
    };
    using enum NullaryError;
    const std::vector<Case> cases = {
        {"1 / 0", Evaluation_DivisionByZero},
        {"1 % 0", Evaluation_DivisionByZero},
        {"1 && 1 / 0", Evaluation_DivisionByZero},
        {"(-32767 - 1) / -1", Evaluation_SignedDivisionOverflow},
        {"(-32767 - 1) % -1", Evaluation_SignedDivisionOverflow},
        {"1 << 16", Evaluation_ShiftOutOfRange},
        {"1 << -1", Evaluation_ShiftOutOfRange},
        {"sym + 1", UnaryError::Symbol_NoValue, "sym"},
    };
    for (const auto &c : cases) {
      CAPTURE(c.source);
      const auto result = evaluate(c.source, pep);
      REQUIRE(!result.has_value());
      CHECK(result.error().matches(c.code, c.argument));
    }

    // Check that errors are localized to the failing text of the expression.
    const auto parse_result = parse("2 + 1 / 0");
    REQUIRE(std::holds_alternative<Parsed>(parse_result));
    const auto &parsed = std::get<Parsed>(parse_result);
    const auto result = evaluate_expression(parsed.tree, pep);
    REQUIRE(!result.has_value());
    const auto division = std::get<Binary>(parsed.tree[parsed.tree.root()]).rhs;
    CHECK(result.error().node == division);
    CHECK(parsed.locations[division].lower() == pepp::tc::support::Location(0, 4));
    CHECK(parsed.locations[division].upper() == pepp::tc::support::Location(0, 9));

    const auto empty = evaluate_expression(Tree{}, pep);
    REQUIRE(!empty.has_value());
    CHECK(!empty.error().node.has_value());
    CHECK(empty.error().matches(NullaryError::Evaluation_Empty));
  }
  SECTION("Function calls") {
    const auto call = [](const char *source) {
      auto result = parse(source, pepp::tc::support::Location(0, 0), nullptr,
                          Options{.percent_identifiers = true, .functions = functions});
      REQUIRE(std::holds_alternative<Parsed>(result));
      return std::get<Parsed>(std::move(result)).tree;
    };
    // A constexpr call is evaluated, and folded, like an operator.
    CHECK(evaluate_expression(call("%twice(3) + 1"), pep).value() == Value{7, {16, Bits}});
    CHECK(to_postfix(fold_constants(call("%twice(3) + sym"), pep)) == "6 sym +");
    CHECK(is_constant_expression(call("%twice(3)")));
    // A  non-constexpr call depends on where it is used, so it is never evaluated.
    CHECK(evaluate_expression(call("%where(3)"), pep).error().matches(UnaryError::Function_NotConstExpr, "%where"));
    CHECK(to_postfix(fold_constants(call("%where(3)"), pep)) == "3 %where");
    CHECK(!is_constant_expression(call("%where(3)")));
  }
  SECTION("Symbols") {
    const ValueOf value_of = [](const Identifier &id) -> std::optional<Value> {
      if (id.name == "sym") return Value{4, {16, Unsigned}};
      return std::nullopt;
    };
    CHECK(evaluate_expression(tree_of("sym + 1"), pep, value_of).value() == Value{5, {16, Unsigned}});
    // The failing node is the symbol which has no value.
    const auto result = evaluate_expression(tree_of("sym + other"), pep, value_of);
    REQUIRE(!result.has_value());
    CHECK(result.error().node == NodeId{1});
    CHECK(result.error().matches(UnaryError::Symbol_NoValue, "other"));

    // The location counter is looked up via symbol table
    Tree here;
    const auto counter = here.add(LocationCounter{"<.0>"});
    here.add(Binary{BinaryOp::Add, counter, here.add(pepp::tc::expr::Integer{1})});
    const ValueOf address = [](const Identifier &id) -> std::optional<Value> {
      if (id.name == "<.0>") return Value{0x10, {16, Unsigned}};
      return std::nullopt;
    };
    CHECK(evaluate_expression(here, pep, address).value() == Value{0x11, {16, Unsigned}});
    CHECK(evaluate_expression(here, pep).error().matches(NullaryError::LocationCounter_NoValue));
    CHECK(!constant_values(here, pep, address)[counter].has_value());
  }
  SECTION("Constant folding") {
    // k stands in for the symbol declared on an .EQUATE and sym for non-constant program location.
    const ValueOf constant_of = [&](const Identifier &id) -> std::optional<Value> {
      if (id.name == "k") return Value{5, {16, Bits}};
      return std::nullopt;
    };
    struct Case {
      const char *source, *folded;
    };
    const std::vector<Case> cases = {
        {"1 + 2", "3"},
        {"0x10", "0x10"},
        {"-4 / 2", "-2"},
        {"0xFFFF + 1", "0"},
        {"sym + (1 + 2)", "sym 3 +"},
        // TODO: will work once we improve our constnat folding algorith with reassociation.
        {"sym + 1 + 2", "sym 1 + 2 +"},
        {"1 + sym", "1 sym +"},
        {"k * 2 + sym", "10 sym +"},
        // Logical operators do not short-circuit while folding unless both are constants.
        {"0 && sym", "0 sym &&"},
        {"1 || sym", "1 sym ||"},
        {"0 && 1", "0"},
        {"0 && 1 / 0", "0 1 0 / &&"},
        {"1 / 0 + sym", "1 0 / sym +"}, // Fails at runtime rather than when constant folding.
    };
    for (const auto &c : cases) {
      CAPTURE(c.source);
      CHECK(to_postfix(fold_constants(tree_of(c.source), pep, constant_of)) == c.folded);
    }

    const auto folded = fold_constants(tree_of("0xFFFF / 2"), pep);
    REQUIRE(std::holds_alternative<FoldedConstant>(folded[folded.root()]));
    CHECK(std::get<FoldedConstant>(folded[folded.root()]).value == Value{0x7FFF, {16, Unsigned}});
    CHECK(evaluate_expression(folded, pep).value() == Value{0x7FFF, {16, Unsigned}});
  }
  SECTION("Node-level constant evaluation and typind") {
    const ValueOf constant_of = [&](const Identifier &id) -> std::optional<Value> {
      if (id.name == "k") return Value{5, {16, Bits}};
      return std::nullopt;
    };
    const TypeOf type_of = [](const Identifier &) { return Type{16, Unsigned}; };
    using Values = std::vector<std::optional<Value>>;
    CHECK(constant_values(tree_of("sym + 2 * k"), pep, constant_of) ==
          Values{std::nullopt, Value{2, {16, Bits}}, Value{5, {16, Bits}}, Value{10, {16, Bits}}, std::nullopt});
    // No short-circuiting, so the division fails and so does everything above it.
    CHECK(constant_values(tree_of("0 && 1 / 0"), pep) ==
          Values{Value{0, {16, Bits}}, Value{1, {16, Bits}}, Value{0, {16, Bits}}, std::nullopt, std::nullopt});

    using Types = std::vector<Type>;
    CHECK(node_types(tree_of("sym + 0x10000"), pep, type_of) == Types{{16, Unsigned}, {32, Bits}, {32, Unsigned}});
    CHECK(node_types(tree_of("-k"), pep, type_of, constant_of) == Types{{16, Bits}, {16, Signed}});
  }
  SECTION("Reassociation") {
    // k stands in for the symbol declared on an .EQUATE, and every other symbol for a 16-bit program location.
    const ValueOf constant_of = [&](const Identifier &id) -> std::optional<Value> {
      if (id.name == "k") return Value{5, {16, Bits}};
      return std::nullopt;
    };
    const TypeOf type_of = [](const Identifier &) { return Type{16, Unsigned}; };
    struct Case {
      const char *source, *reassociated, *folded;
    };
    const std::vector<Case> cases = {
        {"sym + 1 + 2", "1 2 + sym +", "3 sym +"},
        {"1 + sym", "1 sym +", "1 sym +"},
        {"4 * 6 + sym - 5", "4 6 * 5 - sym +", "19 sym +"},
        {"sym - 1 - 2", "0 1 - 2 - sym +", "65533 sym +"}, // Leads with a zero, and wraps modulo 2^16.
        {"5 - sym + 1", "5 1 + sym -", "6 sym -"},
        {"a - b + 1", "1 a + b -", "1 a + b -"}, // Other terms keep their order.
        {"k + sym + 1", "k 1 + sym +", "6 sym +"},
        {"sym * 2 + 1", "1 2 sym * +", "1 2 sym * +"}, // Each chain is reassociated within its own group.
        {"(1 + sym + 2) * 3", "3 1 2 + sym + *", "3 3 sym + *"},
        {"2 * sym * 3", "2 3 * sym *", "6 sym *"},
        {"sym & 0xFF & 0xF", "0xFF 0xF & sym &", "15 sym &"},
        {"sym && 1 && 2", "sym 1 && 2 &&", "sym 1 && 2 &&"}, // Short-circuiting operators keep their order.
        {"1 + 2", "1 2 +", "3"},
        // 0x10000 widens the chain, so sym + 1 must wrap before it is added.
        {"sym + 1 + 0x10000", "1 sym + 0x10000 +", "1 sym + 0x10000 +"},
    };
    for (const auto &c : cases) {
      CAPTURE(c.source);
      const auto reassociated = reassociate_constants(strip_parens(tree_of(c.source)), pep, type_of, constant_of);
      CHECK(to_postfix(reassociated) == c.reassociated);
      CHECK(to_postfix(fold_constants(reassociated, pep, constant_of)) == c.folded);
      CHECK(to_postfix(simplify(tree_of(c.source), pep, type_of, constant_of)) == c.folded);
    }
  }
}
