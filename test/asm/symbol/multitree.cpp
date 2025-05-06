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
#include "toolchain/symbol/visit.hpp"

TEST_CASE("Multiple symbol table trees", "[scope:asm.sym][kind:unit][arch:*]") {
  /*
   *  Test 2 Tree structures
   *          Branch1     Branch2
   *             |        |     |
   *           Leaf1    Leaf2 Leaf3
   */

  SECTION("Independent Existence") {
    auto b1 = QSharedPointer<symbol::Table>::create(2);
    auto b2 = QSharedPointer<symbol::Table>::create(2);
    auto l1 = b1->addChild();
    auto l2 = b2->addChild();
    auto l3 = b2->addChild();
    auto x = l1->reference("x");
    auto y = l2->reference("y");
    auto z = l3->reference("z");

    // Each leaf should find it's own children regardless of policy.
    REQUIRE(symbol::exists(l1, "x"));
    REQUIRE(symbol::exists(l2, "y", symbol::TraversalPolicy::kSiblings));
    REQUIRE(symbol::exists(l3, "z", symbol::TraversalPolicy::kWholeTree));

    // CHECK that traversal policy is respected.
    // Leaf 2 should not be able to see leaf 3's symbol with kChildren.
    // Leaf 1 cannot see leaf 2 or 3
    REQUIRE(!symbol::exists(l2, "z"));
    REQUIRE(!symbol::exists(l3, "y"));
    REQUIRE(!symbol::exists(l1, "z"));
    REQUIRE(!symbol::exists(l1, "y"));

    //  Lower leafs can see other siblings but not another tree with kSiblings
    REQUIRE(symbol::exists(l2, "z", symbol::TraversalPolicy::kSiblings));
    REQUIRE(symbol::exists(l3, "y", symbol::TraversalPolicy::kSiblings));
    REQUIRE(!symbol::exists(l2, "x"));
    REQUIRE(!symbol::exists(l3, "x"));

    //  Lower leafs cannot see other trees even with kWholeTree search
    REQUIRE(!symbol::exists(l2, "x", symbol::TraversalPolicy::kWholeTree));
    REQUIRE(!symbol::exists(l3, "x", symbol::TraversalPolicy::kWholeTree));
    REQUIRE(!symbol::exists(l1, "z", symbol::TraversalPolicy::kWholeTree));
    REQUIRE(!symbol::exists(l1, "y", symbol::TraversalPolicy::kWholeTree));
  }
  SECTION("Independent Globals") {
    auto b1 = QSharedPointer<symbol::Table>::create(2);
    auto b2 = QSharedPointer<symbol::Table>::create(2);
    auto l1 = b1->addChild();
    auto l2 = b2->addChild();
    auto l3 = b2->addChild();
    auto x = l1->reference("hello");
    auto y = l2->reference("hello");

    //  Marking branch leaf 1 as global does not impact branch 2 leaves
    l1->markGlobal("hello");
    CHECK(x->binding == symbol::Binding::kGlobal);
    CHECK(y->binding == symbol::Binding::kLocal);
    CHECK(x->state == symbol::DefinitionState::kUndefined);
    CHECK(y->state == symbol::DefinitionState::kUndefined);

    //  Marking second branch as global does not trigger external multiple error
    //  in other branch
    l2->markGlobal("hello");
    CHECK(x->binding == symbol::Binding::kGlobal);
    CHECK(y->binding == symbol::Binding::kGlobal);
    CHECK(x->state == symbol::DefinitionState::kUndefined);
    CHECK(y->state == symbol::DefinitionState::kUndefined);

    //  Trigger global clash in branch 2
    auto z = l3->reference("hello");
    CHECK(z->binding == symbol::Binding::kImported);
    CHECK(z->state == symbol::DefinitionState::kUndefined);

    //  Create global error with second leaf value marked as global
    l3->markGlobal("hello");
    CHECK(z->binding == symbol::Binding::kGlobal);
    CHECK(x->state == symbol::DefinitionState::kUndefined);
    CHECK(y->state == symbol::DefinitionState::kExternalMultiple);
    CHECK(z->state == symbol::DefinitionState::kExternalMultiple);
  }
}