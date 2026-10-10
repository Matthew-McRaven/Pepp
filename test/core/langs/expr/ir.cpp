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
#include "core/langs/expr/ir.hpp"
#include <catch.hpp>
#include <vector>
#include "core/langs/expr/parser.hpp"

TEST_CASE("Expression parser IR", "[scope:core][scope:core.langs][kind:unit][arch:*]") {
  using namespace bits;
  using namespace pepp::tc::expr;
  using Integer = pepp::tc::expr::Integer;
  using K = Kind;
  const auto tree = [](const char *source) {
    auto result = parse(source);
    REQUIRE(std::holds_alternative<Parsed>(result));
    return std::get<Parsed>(std::move(result)).tree;
  };

  SECTION("Each node type's KIND, narrowed to the operator") {
    STATIC_REQUIRE(Integer::KIND == K::Integer);
    STATIC_REQUIRE(Binary::KIND == K::AnyBinary);
    STATIC_REQUIRE(any(Binary::KIND & kind(BinaryOp::Subtract)));
    STATIC_REQUIRE(none(Unary::KIND & kind(BinaryOp::Subtract)));
    // Postorder: operands before their operator, which is how the nodes are stored.
    CHECK(tree("sym + 1").kinds() == std::vector<K>{K::Identifier, K::Integer, K::Add});
    CHECK(tree("-(a - 'b') * 2").kinds() ==
          std::vector<K>{K::Identifier, K::Character, K::Subtract, K::Parens, K::Minus, K::Integer, K::Multiply});
  }
  SECTION("Tests for integral constant expressions") {
    CHECK(is_constant_expression(tree("-(1 + 'a') * 0x2")));
    CHECK(!is_constant_expression(tree("1 + (2 * sym)")));
    CHECK(is_constant_expression(Tree{}));
    Tree here;
    here.add(LocationCounter{"<.0>"});
    CHECK(!is_constant_expression(here));
  }
  SECTION("Stripping parentheses") {
    const auto stripped = strip_parens(tree("((a + b)) * -(c)"));
    CHECK(to_postfix(stripped) == "a b + c u- *");
    // Fprmatting ass infix re-insert parens needs to eliminate ambiguity.
    CHECK(to_infix(stripped) == "(a + b) * -c");
  }
  SECTION("Pattern matching") {
    // symbol + constant, which is a common pattern for relocations.
    const std::vector<K> sym_plus_const = {K::Identifier, K::Constant, K::Add};
    CHECK(matches(tree("sym + 1").kinds(), sym_plus_const));
    CHECK(matches(tree("sym + 'c'").kinds(), sym_plus_const));
    CHECK(!matches(tree("1 + sym").kinds(), sym_plus_const));     // Operands in the wrong order.
    CHECK(!matches(tree("sym - 1").kinds(), sym_plus_const));     // Wrong operator.
    CHECK(!matches(tree("sym + 1 + 2").kinds(), sym_plus_const)); // Wrong tree structure.
    CHECK(!matches(tree("sym").kinds(), sym_plus_const));

    const std::vector<K> add_or_sub = {K::Identifier, K::Constant, K::Add | K::Subtract};
    CHECK(matches(tree("sym - 1").kinds(), add_or_sub));
    CHECK(matches(tree("1 * 2").kinds(), std::vector<K>{K::Constant, K::Constant, K::AnyBinary}));
  }
}
