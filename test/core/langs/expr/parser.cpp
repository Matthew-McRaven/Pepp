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
#include "core/langs/expr/parser.hpp"
#include <catch.hpp>
#include <array>
#include <expected>
#include <limits>
#include <span>
#include <string>

namespace {
using namespace pepp::tc::expr;
using pepp::tc::expr::Integer;
using pepp::tc::support::Location;

// Constexpr %twice and %add, and non-constexpr %where, which take their first argument's type. ConstEval %defined, and
// %twice_now, a ConstEval %twice.
std::expected<Value, NullaryError> twice(std::span<const Argument> x, Type) {
  const auto value = std::get<Value>(x[0]);
  return Value{value.bits * 2, value.type};
}
std::expected<Value, NullaryError> add(std::span<const Argument> x, Type) {
  const auto lhs = std::get<Value>(x[0]), rhs = std::get<Value>(x[1]);
  return Value{lhs.bits + rhs.bits, lhs.type};
}
std::expected<Value, NullaryError> defined(std::span<const Argument> x, Type default_type) {
  const bool is_defined = std::get<EvaluatedSymbol>(x[0]).state != pepp::core::symbol::DefinitionState::Undefined;
  return Value{is_defined, {default_type.bits, Signedness::Signed}};
}
Type first(std::span<const Type> types, Type) { return types[0]; }
using Category = Function::Category;
constexpr std::array one_value{Function::Parameter::Value};
constexpr std::array two_values{Function::Parameter::Value, Function::Parameter::Value};
constexpr std::array one_symbol{Function::Parameter::Symbol};
const std::array<Function, 5> functions{{
    {"%twice", Category::ConstExpr, one_value, twice, first},
    {"%add", Category::ConstExpr, two_values, add, first},
    {"%where", Category::NonConstExpr, one_value, nullptr, first},
    {"%defined", Category::ConstEval, one_symbol, defined, nullptr},
    {"%twice_now", Category::ConstEval, one_value, twice, nullptr},
}};

Parsed parsed(const ParseResult &result) {
  REQUIRE(std::holds_alternative<Parsed>(result));
  return std::get<Parsed>(result);
}
} // namespace

TEST_CASE("Expression parser", "[scope:core][scope:core.langs][kind:unit][arch:*]") {
  SECTION("left associative C operator precedence") {
    struct Case {
      const char *source, *postfix, *infix;
    };
    const std::vector<Case> cases = {
        {"5-3-1", "5 3 - 1 -", "5 - 3 - 1"},
        {"5-(3-1)", "5 3 1 - -", "5 - (3 - 1)"},
        {"8/4/2", "8 4 / 2 /", "8 / 4 / 2"},
        {"1+2*3", "1 2 3 * +", "1 + 2 * 3"},
        {"(1+2)*3", "1 2 + 3 *", "(1 + 2) * 3"},
        {"1+2<<3", "1 2 + 3 <<", "1 + 2 << 3"},
        {"x>>1<=y", "x 1 >> y <=", "x >> 1 <= y"},
        {"1 < 2 == 3 >= 4", "1 2 < 3 4 >= ==", "1 < 2 == 3 >= 4"},
        {"a&b==c", "a b c == &", "a & b == c"},
        {"a|b^c&d", "a b c d & ^ |", "a | b ^ c & d"},
        {"a||b&&c", "a b c && ||", "a || b && c"},
        {"-a*b", "a u- b *", "-a * b"},
        {"-(a+b)", "a b + u-", "-(a + b)"},
        {"!~-x", "x u- ~ !", "!~-x"},
        {"a - -b", "a b u- -", "a - -b"},
        {"+a % (b - c)", "a u+ b c - %", "+a % (b - c)"},
        {"((a))", "a", "((a))"}, // Parentheses are kept as written, even when redundant.
        {"(5+6)", "5 6 +", "(5 + 6)"},
        {"0x1f * '\\n'", "0x1F '\\n' *", "0x1F * '\\n'"}, // Literals print in their own format.
    };
    for (const auto &c : cases) {
      CAPTURE(c.source);
      const auto parse_result = parse(c.source);
      const auto &result = parsed(parse_result);
      CHECK(to_postfix(result.tree) == c.postfix);
      CHECK(to_infix(result.tree) == c.infix);
      CHECK(result.length == std::string_view(c.source).size());
    }
  }
  SECTION("Integer literals") {
    using Integer = pepp::tc::expr::Integer;
    using Format = Integer::Format;
    constexpr u64 max = std::numeric_limits<u64>::max();
    struct IntegerCase {
      const char *source;
      u64 value;
      Format format;
    };
    const std::vector<IntegerCase> integers = {
        {"42", 42, Format::Decimal},
        {"0x1F", 0x1F, Format::Hexadecimal},
        {"0XfF", 0xFF, Format::Hexadecimal},
        {"18446744073709551615", max, Format::Decimal},
        {"0xFFFFFFFFFFFFFFFF", max, Format::Hexadecimal},
    };
    for (const auto &c : integers) {
      CAPTURE(c.source);
      const auto parse_result = parse(c.source);
      const auto &result = parsed(parse_result);
      REQUIRE(std::holds_alternative<Integer>(result.tree[result.tree.root()]));
      const auto &integer = std::get<Integer>(result.tree[result.tree.root()]);
      CHECK(integer.value == c.value);
      CHECK(integer.format == c.format);
    }
    struct CharacterCase {
      const char *source;
      u8 value;
      const char *text;
    };
    // Check that escape sequences are preserved, even if there is an equivalent ASCII character.
    const std::vector<CharacterCase> characters = {
        {"'a'", 'a', "a"},      {"'\\n'", '\n', "\\n"}, {"'\\x41'", 0x41, "\\x41"},
        {"'\\''", '\'', "\\'"}, {"'\\0'", 0, "\\0"},    {"'\\\\'", '\\', "\\\\"},
    };
    for (const auto &c : characters) {
      CAPTURE(c.source);
      const auto parse_result = parse(c.source);
      const auto &result = parsed(parse_result);
      REQUIRE(std::holds_alternative<Character>(result.tree[result.tree.root()]));
      const auto &character = std::get<Character>(result.tree[result.tree.root()]);
      CHECK(character.value == c.value);
      CHECK(character.text == c.text);
    }
  }
  SECTION("Halt on first unconsumable token") {
    struct Case {
      const char *source;
      size_t length;
      const char *shape;
    };
    const std::vector<Case> cases = {
        {"a+1,d", 3, "a 1 +"},
        {"8(x2)", 1, "8"},
        {"  sym + 2 ;comment", 9, "sym 2 +"},
        {"a - -3, i", 6, "a 3 u- -"},
        {"(1 + 2) * 3;", 11, "1 2 + 3 *"},
        {"a b", 1, "a"},
        {"a\nb", 1, "a"},
        {"1 = 2", 1, "1"},
        {"x ~y", 1, "x"}, // Currently unimplemented
    };
    for (const auto &c : cases) {
      CAPTURE(c.source);
      const auto parse_result = parse(c.source);
      const auto &result = parsed(parse_result);
      CHECK(result.length == c.length);
      CHECK(to_postfix(result.tree) == c.shape);
      // The returned cursor points immediately after the matched expression.
      CHECK(result.after.rest() == std::string_view(c.source).substr(c.length));
      CHECK(result.after.location() == Location(0, static_cast<u16>(c.length)));
    }
  }
  SECTION("location counter") {
    using Dot = Options::Dot;
    int named = 0;
    const NameLocationCounter name = [&] { return "<." + std::to_string(named++) + ">"; };
    const auto result = parse(". + 4 - .", Location(0, 0), nullptr, {.dot = Dot::Identifier}, name);
    const auto &tree = parsed(result).tree;
    CHECK(to_postfix(tree) == ". 4 + . -");
    CHECK(to_infix(tree) == ". + 4 - .");
    // All . in an expression refer to the same symbol
    CHECK(named == 1);
    CHECK(std::get<LocationCounter>(tree[0]).name == "<.0>");
    CHECK(std::get<LocationCounter>(tree[3]).name == "<.0>");

    // . is conditionally enabled in the grammar
    CHECK(std::holds_alternative<NoExpression>(parse(".")));
    const auto unnamed = parse(".", Location(0, 0), nullptr, {.dot = Dot::Identifier});
    REQUIRE(std::holds_alternative<Error>(unnamed));
    CHECK(std::get<Error>(unnamed).matches(NullaryError::LocationCounter_Unavailable));
    const auto member = parse("a.b", Location(0, 0), nullptr, {.dot = Dot::Operator});
    REQUIRE(std::holds_alternative<Error>(member));
    CHECK(std::get<Error>(member).matches(NullaryError::Syntax_MemberAccess));
  }
  SECTION("Function calls") {
    const Options options{.percent_identifiers = true, .functions = functions};
    const auto call = [&](const char *source) { return parse(source, Location(0, 0), nullptr, options); };
    const auto &tree = parsed(call("%twice(1 + 2) * 3")).tree;
    CHECK(to_postfix(tree) == "1 2 + %twice 3 *");
    CHECK(to_infix(tree) == "%twice(1 + 2) * 3");
    CHECK(tree.kinds()[3] == Kind::Call);
    CHECK(parsed(call("%where(x)")).tree.kinds()[1] == Kind::Call);
    const auto &pair = parsed(call("%add(1, 2 * 3)")).tree;
    CHECK(to_infix(pair) == "%add(1, 2 * 3)");
    CHECK(to_postfix(pair) == "1 2 3 * %add");
    const auto short_by_one_arg = call("%add(1)");
    REQUIRE(std::holds_alternative<Error>(short_by_one_arg));
    CHECK(std::get<Error>(short_by_one_arg).matches(UnaryError::Function_WrongArity, "%add"));
    // Identifiers with % must be functions
    const auto unknown = call("%nope(1)"), uncalled = call("%twice 1");
    REQUIRE(std::holds_alternative<Error>(unknown));
    CHECK(std::get<Error>(unknown).matches(UnaryError::Function_Unknown, "%nope"));
    REQUIRE(std::holds_alternative<Error>(uncalled));
    CHECK(std::get<Error>(uncalled).matches(NullaryError::Syntax_ExpectedOpenParen));
  }
  SECTION("ConstEval calls") {
    using pepp::core::symbol::DefinitionState;
    const Options options{.percent_identifiers = true, .functions = functions};
    // Only k is defined so far, as a constant.
    const ValueOf constant_of = [](const Identifier &id) -> EvaluatedSymbol {
      if (id.name == "k") return {DefinitionState::Single, Value{5, {32, Signedness::Bits}}};
      return {};
    };
    const auto call = [&](const char *source) {
      return parse(source, Location(0, 0), nullptr, options, {}, constant_of);
    };
    // ConstEval calls evaluated as they are parsed
    const auto symbols = parsed(call("%defined(k) + %defined(m)"));
    CHECK(symbols.tree.kinds() == std::vector<Kind>{Kind::FoldedConstant, Kind::FoldedConstant, Kind::Add});
    CHECK(symbols.locations.size() == 3);
    CHECK(std::get<FoldedConstant>(symbols.tree[0]).value == Value{1, {32, Signedness::Signed}});
    CHECK(std::get<FoldedConstant>(symbols.tree[1]).value == Value{0, {32, Signedness::Signed}});
    // A Value argument is evaluated from the constants so far.
    const auto values = parsed(call("%twice_now(k+1)"));
    CHECK(values.tree.kinds() == std::vector<Kind>{Kind::FoldedConstant});
    CHECK(to_infix(values.tree) == "%twice_now(k + 1)");
    CHECK(std::get<FoldedConstant>(values.tree[0]).value.bits == 12);

    const auto error = [&](const char *source) {
      auto result = call(source);
      REQUIRE(std::holds_alternative<Error>(result));
      return std::get<Error>(result);
    };
    // Symbol arguments must be literal identifiers, not expressions involving them
    for (const char *source : {"%defined(1)", "%defined(k + 1)", "%defined((k))", "%defined(%twice(k))"}) {
      CAPTURE(source);
      const auto e = error(source);
      CHECK(e.matches(UnaryError::Function_ExpectedSymbol, "%defined"));
      CHECK(e.location.lower() == Location(0, 9));
    }
    CHECK(error("%twice_now(m)").matches(UnaryError::Symbol_NoValue, "m"));
    // No constant_of means all symbols are treated as undefined.
    const auto unavailable = parse("%defined(k)", Location(0, 0), nullptr, options);
    REQUIRE(std::holds_alternative<Error>(unavailable));
    CHECK(std::get<Error>(unavailable).matches(UnaryError::Function_Unavailable, "%defined"));
  }
  SECTION("Consume no input if the text does not start with an expression") {
    for (const char *source :
         {"", "   ", ",d", "\"str\"", ")", "\n", "*x", "; comment", "'ab'", "''", "99999999999999999999", "0x"}) {
      CAPTURE(source);
      CHECK(std::holds_alternative<NoExpression>(parse(source)));
    }
  }
  SECTION("Report true syntax errors") {
    struct Case {
      const char *source;
      u16 lower, upper; // Columns of the offending token.
      NullaryError code;
    };
    using enum NullaryError;
    const std::vector<Case> cases = {
        {"a + ,d", 4, 5, Syntax_ExpectedOperand},  {"a +", 3, 3, Syntax_ExpectedOperand},
        {"a + * b", 4, 5, Syntax_ExpectedOperand}, {"-", 1, 1, Syntax_ExpectedOperand},
        {"(a + 1", 6, 6, Syntax_ExpectedCloseParen}, {"(1 + 2 ,", 7, 8, Syntax_ExpectedCloseParen},
        {"1 + 0x", 4, 6, Syntax_ExpectedOperand},  {"1 + 99999999999999999999", 4, 24, Syntax_ExpectedOperand},
    };
    for (const auto &c : cases) {
      CAPTURE(c.source);
      const auto result = parse(c.source);
      REQUIRE(std::holds_alternative<Error>(result));
      const auto &error = std::get<Error>(result);
      CHECK(error.location.lower() == Location(0, c.lower));
      CHECK(error.location.upper() == Location(0, c.upper));
      CHECK(error.matches(c.code));
    }
  }
  SECTION("Locations are relative to the origin") {
    const auto parse_result = parse("(a) + bb", Location(3, 10));
    const auto &result = parsed(parse_result);
    const auto &tree = result.tree;
    const auto &locations = result.locations;
    REQUIRE(locations.size() == tree.nodes().size());
    CHECK(locations[tree.root()].lower() == Location(3, 10));
    CHECK(locations[tree.root()].upper() == Location(3, 18));
    const auto &sum = std::get<Binary>(tree[tree.root()]);
    CHECK(locations[sum.rhs].lower() == Location(3, 16));
    CHECK(locations[sum.rhs].upper() == Location(3, 18));
    // A Parens node's span includes its parentheses, but its operand's does not.
    CHECK(locations[sum.lhs].lower() == Location(3, 10));
    CHECK(locations[sum.lhs].upper() == Location(3, 13));
    const auto inner = std::get<Parens>(tree[sum.lhs]).inner;
    CHECK(locations[inner].lower() == Location(3, 11));
    CHECK(locations[inner].upper() == Location(3, 12));
  }
}
