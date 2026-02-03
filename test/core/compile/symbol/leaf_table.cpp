/*
 * Copyright (c) 2023-2026 J. Stanley Warford, Matthew McRaven
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

#include "core/compile/symbol/leaf_table.hpp"
#include <catch/catch.hpp>
#include "core/compile/symbol/entry.hpp"
#include "core/compile/symbol/value.hpp"

TEST_CASE("Leaf symbol tables", "[scope:core][scope:core.compile][kind:unit][arch:*]") {
  using namespace pepp::core::symbol;
  SECTION("successive reference() return the same object") {
    auto st = std::make_shared<pepp::core::symbol::LeafTable>(2);
    auto x = st->reference("hello");
    auto y = st->reference("hello");
    CHECK(x == y);
  }
  SECTION("get() and reference() return the same object") {
    auto st = std::make_shared<pepp::core::symbol::LeafTable>(2);
    auto z = st->get("hello");
    CHECK(z == std::nullopt);
    auto x = st->reference("hello");
    auto y = st->get("hello");
    CHECK(x == y);
  }
  SECTION("names are case sensitive") {
    auto st = std::make_shared<pepp::core::symbol::LeafTable>(2);
    auto x = st->reference("hello");
    auto y = st->reference("Hello");
    CHECK(x != y);
  }
  SECTION("exist() checks are correct and case sensitive") {
    auto st = std::make_shared<pepp::core::symbol::LeafTable>(2);
    st->reference("hello");
    REQUIRE(!st->exists("bye"));
    REQUIRE(!st->exists("Hello"));
    REQUIRE(st->exists("hello"));
  }

  SECTION("define() increments definition state") {
    auto st = std::make_shared<pepp::core::symbol::LeafTable>(2);
    auto x = st->reference("hello");
    CHECK(x->state == DefinitionState::Undefined);
    st->define(x->name);
    CHECK(x->state == DefinitionState::Single);
    st->define(x->name);
    CHECK(x->state == DefinitionState::Multiple);
  }
  SECTION("define() and reference() return the same object") {
    auto st = std::make_shared<pepp::core::symbol::LeafTable>(2);
    st->define("hello");
    auto x = st->reference("hello");
    CHECK(x->state == DefinitionState::Single);
  }

  SECTION("define() returns the same object on successive calls") {
    auto st = std::make_shared<pepp::core::symbol::LeafTable>(2);
    auto x = st->define("hello");
    auto y = st->define("hello");
    CHECK(x->state == DefinitionState::Multiple);
    CHECK(x == y);
  }
  SECTION("define and get() return the same object") {
    auto st = std::make_shared<pepp::core::symbol::LeafTable>(2);
    auto z = st->get("hello");
    CHECK(z == std::nullopt);
    auto x = st->define("hello");
    auto y = st->get("hello");
    CHECK(x == y);
  }
  SECTION("define() is case sensitive") {
    auto st = std::make_shared<pepp::core::symbol::LeafTable>(2);
    auto x = st->define("hello");
    auto y = st->define("Hello");
    CHECK(x != y);
  }

  SECTION("Multiple references do not increment definiton count") {
    for (int it = 0; it < 4; it++) {
      auto st = std::make_shared<pepp::core::symbol::LeafTable>(2);
      for (int i = 0; i < it; i++) st->reference("hello");
      CHECK(st->reference("hello")->state == DefinitionState::Undefined);
    }
  }
  SECTION("Multiple defines increment definition count") {
    for (int it = 2; it < 4; it++) {
      auto st = std::make_shared<pepp::core::symbol::LeafTable>(2);
      for (int i = 0; i < it; i++) st->define("hello");
      CHECK(st->reference("hello")->state == DefinitionState::Multiple);
    }
  }
  SECTION("adjust_offset() respects threshold") {
    auto st = std::make_shared<pepp::core::symbol::LeafTable>(2);
    auto x0 = st->define("h0");
    auto x1 = st->define("h1");
    auto x2 = st->define("h2");
    auto x3 = st->define("h3");

    x0->value = std::make_shared<pepp::core::symbol::LocationValue>(2, 2, 10, 0, Type::Object);
    x1->value = std::make_shared<pepp::core::symbol::LocationValue>(2, 2, 20, 0, Type::Object);
    x2->value = std::make_shared<pepp::core::symbol::LocationValue>(2, 2, 30, 0, Type::Code);
    x3->value = std::make_shared<pepp::core::symbol::LocationValue>(2, 2, 9, 0, Type::Code);
    increment_offset(*st, 1, 10);

    CHECK(x0->value->value()() == 11);
    CHECK(x1->value->value()() == 21);
    CHECK(x2->value->value()() == 31);
    CHECK(x3->value->value()() == 9);
  }
  SECTION("table_listing() does not throw") {
    auto st = std::make_shared<pepp::core::symbol::LeafTable>(2);
    auto x0 = st->define("h0");
    auto x1 = st->define("h1");
    auto x2 = st->define("h2");
    REQUIRE_NOTHROW(table_listing(*st, 2));
  }
}
