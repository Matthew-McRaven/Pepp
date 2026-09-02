
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
#include "toolchain/symbol/visit.hpp"

TEST_CASE("Two symbol tables", "[scope:asm.sym][kind:unit][arch:*]") {
  SECTION("Independent References") {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto st1 = st->addChild();
    auto st2 = st->addChild();
    auto x = st1->reference("hello");
    auto y = st2->reference("hello");
    // 2. 1 for local copy, 1 in map.
    // CHECK(x.use_count() == 2);
    CHECK(x != y);
  }
  SECTION("Find by name") {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto st1 = st->addChild();
    auto st2 = st->addChild();
    auto x = st1->reference("hello");
    auto y = st1->reference("hello");
    auto z = st2->reference("hello");
    CHECK(x == y);
    // CHECK that reference doesn't leak over.
    CHECK(z != x);
    CHECK(z != y);
  }
  SECTION("Get by name") {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto st1 = st->addChild();
    auto st2 = st->addChild();
    auto x = st1->get("hello");
    CHECK(x == std::nullopt);
    auto y = st1->get("hello");
    CHECK(y == std::nullopt);
    auto x1 = st1->reference("hello");
    auto x2 = st1->get("hello");
    CHECK(x1 == x2);
    auto y1 = st2->define("hello"); //  Uses define instead of reference
    auto y2 = st2->get("hello");
    CHECK(y1 == y2);
    CHECK(x1 != y1);
  }
  SECTION("Existence checks") {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto st1 = st->addChild();
    auto st2 = st->addChild();
    // Discard reference returned by st.
    st1->reference("hello");
    // CHECK that traversal policy is respected.
    // Table 2 should not be able to see table 1's symbol with kChildren.
    REQUIRE(!symbol::exists(st2, "hello"));
    // But it can see them when it is allowed to CHECK siblings.
    REQUIRE(symbol::exists(st2, "hello", symbol::TraversalPolicy::kSiblings));
    // Trivially the root can always see any symbol.
    REQUIRE(symbol::exists(st, "hello"));
  }
  SECTION("Existence respects traversal policy") {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto st1 = st->addChild();
    auto st2 = st->addChild();
    st1->reference("hello");
    // CHECK that traversal policy is respected.
    // This calls exists from table directly
    REQUIRE(st1->exists("hello"));
    REQUIRE(!st2->exists("hello"));
  }
  SECTION("Locality of defines") {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto st1 = st->addChild();
    auto st2 = st->addChild();
    auto x = st1->reference("hello");
    auto y = st2->reference("hello");
    CHECK(x->state == symbol::DefinitionState::kUndefined);
    st1->define(x->name);
    CHECK(x->state == symbol::DefinitionState::kSingle);
    st1->define(x->name);
    CHECK(x->state == symbol::DefinitionState::kMultiple);
    // Defining a local symbol doesn't affect the state of a symbol in another
    // table.
    CHECK(y->state == symbol::DefinitionState::kUndefined);
  }
  SECTION("Single valid global") {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto st1 = st->addChild();
    auto st2 = st->addChild();
    auto x = st1->reference("hello");
    auto y = st2->reference("hello");
    st1->markGlobal("hello");
    CHECK(x->binding == symbol::Binding::kGlobal);
    CHECK(y->binding == symbol::Binding::kImported);
    CHECK(x->state == symbol::DefinitionState::kUndefined);
    CHECK(y->state == symbol::DefinitionState::kUndefined);
    // CHECK that defining a global symbol also defines its imports.
    st1->define("hello");
    CHECK(x->state == symbol::DefinitionState::kSingle);
    CHECK(y->state == symbol::DefinitionState::kSingle);
    st2->define("hello");
    CHECK(x->state == symbol::DefinitionState::kSingle);
    CHECK(y->state == symbol::DefinitionState::kExternalMultiple);
  }
  SECTION("Multiple globals") {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto st1 = st->addChild();
    auto st2 = st->addChild();
    auto x = st1->reference("hello");
    auto y = st2->reference("hello");
    st1->markGlobal("hello");
    st2->markGlobal("hello");
    CHECK(x->binding == symbol::Binding::kGlobal);
    CHECK(y->binding == symbol::Binding::kGlobal);
    CHECK(x->state == symbol::DefinitionState::kExternalMultiple);
    CHECK(y->state == symbol::DefinitionState::kExternalMultiple);
  }
  SECTION("Extenal table") {
    auto st1 = QSharedPointer<symbol::Table>::create(2);
    auto st2 = QSharedPointer<symbol::Table>::create(2);

    auto x = st1->define("hello");
    auto x_val = QSharedPointer<symbol::value::Constant>::create();
    x_val->setValue({.byteCount = 2, .bitPattern = 0xfeed, .mask = 0x8FFF});
    x->value = x_val;
    st1->markGlobal("hello");

    auto maybe_y = st2->import(*st1, "hello");
    REQUIRE(maybe_y.has_value());
    auto y = maybe_y.value();
    CHECK(x->binding == symbol::Binding::kGlobal);
    CHECK(y->binding == symbol::Binding::kImported);
    CHECK(x->state == symbol::DefinitionState::kSingle);
    CHECK(y->state == symbol::DefinitionState::kSingle);
    CHECK(x->value->value() == y->value->value());
  }
}
