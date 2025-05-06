/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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
#include "toolchain/symbol/table.hpp"
#include "toolchain/symbol/types.hpp"
#include "toolchain/symbol/value.hpp"
#include "toolchain/symbol/visit.hpp"

TEST_CASE("Single symbol table", "[scope:asm.sym][kind:unit][arch:*]") {
  SECTION("Find by name") {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto x = st->reference("hello");
    auto y = st->reference("hello");
    CHECK(x == y);
  }
  //  Dave: Added get tests
  SECTION("Get by name") {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto z = st->get("hello");
    CHECK(z == std::nullopt);
    auto x = st->reference("hello");
    auto y = st->get("hello");
    CHECK(x == y);
  }
  SECTION("Get by reference") {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto x = st->reference("hello");
    auto y = st->reference("Hello");
    CHECK(x != y);
  }
  SECTION("Case sensitive") {
    auto st = QSharedPointer<symbol::Table>::create(2);
    // Discard reference returned by st.
    st->reference("hello");
    //  Uses visitor pattern in visit.hpp
    REQUIRE(!symbol::exists(st, "bye"));
    REQUIRE(!symbol::exists(st, "Hello"));
    REQUIRE(symbol::exists(st, "hello"));
  }
  SECTION("Existence checks, table method1") {
    auto st = QSharedPointer<symbol::Table>::create(2);
    st->reference("hello");
    //  Uses table specific exists function
    REQUIRE(!st->exists("bye"));
    REQUIRE(!st->exists("Hello"));
    REQUIRE(st->exists("hello"));
  }
  SECTION("Existence checks, free function") {
    auto st = QSharedPointer<symbol::Table>::create(2);
    // Discard reference returned by st.
    st->reference("hello");
    //  Uses visitor pattern in visit.hpp
    REQUIRE(!symbol::exists(st, "bye"));
    REQUIRE(!symbol::exists(st, "Hello"));
    REQUIRE(symbol::exists(st, "hello"));
  }
  SECTION("Progresses through definition states") {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto x = st->reference("hello");
    CHECK(x->state == symbol::DefinitionState::kUndefined);
    st->define(x->name);
    CHECK(x->state == symbol::DefinitionState::kSingle);
    // CHECK(x.use_count() == 2);
    st->define(x->name);
    CHECK(x->state == symbol::DefinitionState::kMultiple);
    // CHECK(x.use_count() == 2);
  }
  SECTION("Define before reference") {
    auto st = QSharedPointer<symbol::Table>::create(2);
    st->define("hello");
    auto x = st->reference("hello");
    CHECK(x->state == symbol::DefinitionState::kSingle);
  }
  // Yes, the symbols are multiple-defined, but need to ensure they point to
  // same symbol.
  //  Dave: Test that define works like Reference
  SECTION("Define finds by name") {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto x = st->define("hello");
    auto y = st->define("hello");
    CHECK(x->state == symbol::DefinitionState::kMultiple);
    CHECK(x == y);
  }
  SECTION("Define and get") {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto z = st->get("hello");
    CHECK(z == std::nullopt);
    auto x = st->define("hello");
    auto y = st->get("hello");
    CHECK(x == y);
  }
  SECTION("Define is case sensitive") {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto x = st->define("hello");
    auto y = st->define("Hello");
    CHECK(x != y);
  }
  SECTION("Define then exists") {
    auto st = QSharedPointer<symbol::Table>::create(2);
    st->define("hello");
    REQUIRE(!st->exists("bye"));
    REQUIRE(!st->exists("Hello"));
    REQUIRE(st->exists("hello"));
  }
  SECTION("Multiple references") {
    for (int it = 0; it < 4; it++) {
      auto st = QSharedPointer<symbol::Table>::create(2);
      for (int i = 0; i < it; i++) st->reference("hello");
      CHECK(st->reference("hello")->state == symbol::DefinitionState::kUndefined);
    }
  }
  SECTION("Multiple defines") {
    for (int it = 2; it < 4; it++) {
      auto st = QSharedPointer<symbol::Table>::create(2);
      for (int i = 0; i < it; i++) st->define("hello");
      CHECK(st->reference("hello")->state == symbol::DefinitionState::kMultiple);
    }
  }
  SECTION("Global offset modification") {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto x0 = st->define("h0");
    auto x1 = st->define("h1");
    auto x2 = st->define("h2");

    x0->value = QSharedPointer<symbol::value::Location>::create(2, 2, 10, 0, symbol::Type::kObject);
    x1->value = QSharedPointer<symbol::value::Location>::create(2, 2, 20, 0, symbol::Type::kObject);
    x2->value = QSharedPointer<symbol::value::Location>::create(2, 2, 30, 0, symbol::Type::kCode);
    symbol::adjustOffset(st, 1, 10);

    CHECK(x0->value->value()() == 11);
    CHECK(x1->value->value()() == 21);
    CHECK(x2->value->value()() == 31);
  }
  SECTION("Listing does not throw") {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto x0 = st->define("h0");
    auto x1 = st->define("h1");
    auto x2 = st->define("h2");
    REQUIRE_NOTHROW(symbol::tableListing(st, 2));
  }
  SECTION("Offset above threshold") {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto x0 = st->define("h0");
    auto x1 = st->define("h1");
    auto x2 = st->define("h2");

    x0->value = QSharedPointer<symbol::value::Location>::create(2, 2, 10, 0, symbol::Type::kObject);
    x1->value = QSharedPointer<symbol::value::Location>::create(2, 2, 20, 0, symbol::Type::kObject);
    x2->value = QSharedPointer<symbol::value::Location>::create(2, 2, 30, 0, symbol::Type::kCode);
    symbol::adjustOffset(st, -1, 12);

    CHECK(x0->value->value()() == 10);
    CHECK(x1->value->value()() == 19);
    CHECK(x2->value->value()() == 29);
  }
  SECTION("markGlobal is idempotent") {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto x = st->reference("hello");
    st->markGlobal("hello");
    st->markGlobal("hello");
    CHECK(x->binding == symbol::Binding::kGlobal);
    CHECK(x->state == symbol::DefinitionState::kUndefined);
  }
  SECTION("Define then mark global") {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto x = st->define("hello");
    st->markGlobal("hello");
    st->markGlobal("hello"); //  Ignored
    CHECK(x->binding == symbol::Binding::kGlobal);
    CHECK(x->state == symbol::DefinitionState::kSingle);
  }
}
