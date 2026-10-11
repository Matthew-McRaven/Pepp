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

#include "core/compile/ir_value/expression.hpp"
#include <array>
#include <catch.hpp>
#include <memory>
#include "core/compile/symbol/entry.hpp"
#include "core/compile/symbol/leaf_table.hpp"
#include "core/compile/symbol/value.hpp"
#include "core/langs/expr/parser.hpp"

namespace {
using pepp::ast::Expression;
using namespace pepp::tc::expr;
using pepp::core::symbol::LeafTable;

Tree tree_of(const char *source) {
  auto result = parse(source);
  REQUIRE(std::holds_alternative<Parsed>(result));
  return std::get<Parsed>(std::move(result)).tree;
}
} // namespace

TEST_CASE("pepp expressions", "[scope:core][scope:core.compile][kind:unit][arch:*]") {
  // Pep/10 has a 16-bit int and treats numbers as unsigned by default.
  const Type pep{16, Signedness::Unsigned};
  auto symtab = std::make_shared<LeafTable>(2);

  SECTION("Serializing and sizing") {
    symtab->reference("l")->value =
        std::make_shared<pepp::core::symbol::LocationValue>(1, 2, 0, 0, pepp::core::symbol::Type::Object);
    const Expression expression(tree_of("(l - 1) * 2"), symtab, pep, 2);
    CHECK(expression.serialized_size() == 2);
    CHECK(expression.string() == "(l - 1) * 2");
    CHECK(expression.value_as<u16>() == 0xFFFE);
    // 0xFFFE fits in one byte when read as signed.
    CHECK(expression.minimum_size() == 1);
    std::array<u8, 2> bytes{};
    CHECK(expression.serialize(bytes, bits::Order::BigEndian) == 2);
    CHECK(bytes == std::array<u8, 2>{0xFF, 0xFE});
  }
  SECTION("Failures to evaluate yield default values") {
    const Expression expression(tree_of("1 / 0"), nullptr, pep, 2);
    CHECK(!expression.evaluate().has_value());
    CHECK(expression.value_as<u16>() == 0);
    CHECK(expression.minimum_size() == 2);
  }
  SECTION("Undefined symbols read as 0") {
    (void)symtab->reference("u");
    const Expression expression(tree_of("u - 2"), symtab, pep, 2);
    CHECK(expression.evaluate().value() == Value{0xFFFE, {16, Signedness::Unsigned}});
  }
  SECTION("Symbols missing from the table have no value") {
    const Expression expression(tree_of("missing - 2"), symtab, pep, 2);
    const auto result = expression.evaluate();
    REQUIRE(!result.has_value());
    CHECK(result.error().matches(pepp::tc::expr::UnaryError::Symbol_NoValue, "missing"));
    CHECK(expression.value_as<u16>() == 0);
  }
  SECTION("contains_symbol works as expected") {
    CHECK(contains_symbol(Expression(tree_of("l + 1"), symtab, pep, 2)));
    CHECK(!contains_symbol(Expression(tree_of("2 + 1"), symtab, pep, 2)));
  }
}
