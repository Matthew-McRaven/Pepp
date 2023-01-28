#include "catch.hpp"

#include "symbol/table.hpp"
#include "symbol/visit.hpp"

/*
 *  Test Tree structure
 *          Branch (Parent)
 *          |             |
 *          Branch2     Leaf1
 *          |     |
 *        Leaf2 Leaf3
 */
TEST_CASE("Validate functionality for multiple symbol tables.") {

  SECTION("Check that local references are independent.") {
    auto b1 = QSharedPointer<symbol::Table>::create();
    auto b2 = b1->addChild();
    auto l1 = b1->addChild();
    auto l2 = b2->addChild();
    auto l3 = b2->addChild();
    auto x = l1->reference("hello");
    auto y = l2->reference("hello");
    auto z = l3->reference("hello");
    // 2. 1 for local copy, 1 in map.
    // CHECK(x.use_count() == 2);
    // CHECK(y.use_count() == 2);
    // CHECK(z.use_count() == 2);
    CHECK(x != y);
    CHECK(x != z);
    CHECK(y != z);
  }

  SECTION("Find by name.") {
    auto b1 = QSharedPointer<symbol::Table>::create();
    auto b2 = b1->addChild();
    auto l1 = b1->addChild();
    auto l2 = b2->addChild();
    auto l3 = b2->addChild();
    auto x1 = l1->reference("hello");
    auto x2 = l1->reference("hello");
    auto y1 = l2->reference("hello");
    auto y2 = l2->reference("hello");
    auto z1 = l3->reference("hello");
    auto z2 = l3->reference("hello");
    CHECK(x1 == x2);
    CHECK(y1 == y2);
    CHECK(z1 == z2);
    // Check that reference doesn't leak over.
    CHECK(x1 != y1);
    CHECK(x2 != z2);
    CHECK(y2 != z1);
  }

    //  Dave: Added get tests
  SECTION("Get by name using reference.") {
    auto b1 = QSharedPointer<symbol::Table>::create();
    auto b2 = b1->addChild();
    auto l1 = b1->addChild();
    auto l2 = b2->addChild();
    auto l3 = b2->addChild();
    auto x = l1->get("hello");
    CHECK(x == std::nullopt);
    auto y = l2->get("hello");
    CHECK(y == std::nullopt);
    auto z = l3->get("hello");
    CHECK(z == std::nullopt);
    auto x1 = l1->reference("hello");
    auto x2 = l1->get("hello");
    CHECK(x2 == x1);
    auto y1 = l2->define("hello");   //  Uses define instead of reference
    auto y2 = l2->get("hello");
    CHECK(y2 == y1);
    auto z1 = l3->define("hello");   //  Uses define instead of reference
    auto z2 = l3->get("hello");
    CHECK(z2 == z2);
  }

  SECTION("Symbol existence checks.") {
    auto b1 = QSharedPointer<symbol::Table>::create();
    auto b2 = b1->addChild();
    auto l1 = b1->addChild();
    auto l2 = b2->addChild();
    auto l3 = b2->addChild();
    auto x = l1->reference("x");
    auto y = l2->reference("y");
    auto z = l3->reference("z");

    // Each leaf should find it's own children regardless of policy.
    CHECK(symbol::exists(l1, "x"));
    CHECK(symbol::exists(l2, "y", symbol::TraversalPolicy::kSiblings));
    CHECK(symbol::exists(l3, "z", symbol::TraversalPolicy::kWholeTree));

    // Check that traversal policy is respected.
    // Leaf 2 should not be able to see leaf 3's symbol with kChildren.
    // Parent leaf can see all others
    CHECK_FALSE(symbol::exists(l2, "z"));
    CHECK_FALSE(symbol::exists(l3, "y"));
    CHECK_FALSE(symbol::exists(l1, "z"));
    CHECK_FALSE(symbol::exists(l1, "y"));

    //  Lower leafs can see other siblings but not parent with kSiblings
    CHECK(symbol::exists(l2, "z", symbol::TraversalPolicy::kSiblings));
    CHECK(symbol::exists(l3, "y", symbol::TraversalPolicy::kSiblings));
    CHECK_FALSE(symbol::exists(l2, "x", symbol::TraversalPolicy::kSiblings));
    CHECK_FALSE(symbol::exists(l3, "x", symbol::TraversalPolicy::kSiblings));

    //  Lower leafs can see parent and siblings
    CHECK(symbol::exists(l2, "x", symbol::TraversalPolicy::kWholeTree));
    CHECK(symbol::exists(l3, "x", symbol::TraversalPolicy::kWholeTree));
    CHECK(symbol::exists(l1, "z", symbol::TraversalPolicy::kWholeTree));
    CHECK(symbol::exists(l1, "y", symbol::TraversalPolicy::kWholeTree));
  }

  SECTION("define() a local in one table does not affect the other.") {
    auto b1 = QSharedPointer<symbol::Table>::create();
    auto b2 = b1->addChild();
    auto l1 = b1->addChild();
    auto l2 = b2->addChild();
    auto l3 = b2->addChild();
    auto x = l1->reference("hello");
    auto y = l2->define("hello");
    auto z = l3->reference("hello");

    CHECK(x->state == symbol::DefinitionState::kUndefined);
    l1->define(x->name);
    CHECK(x->state == symbol::DefinitionState::kSingle);
    l1->define(x->name);
    CHECK(x->state == symbol::DefinitionState::kMultiple);
    // Y was created and defined in one step. Check state
    CHECK(y->state == symbol::DefinitionState::kSingle);
    l2->define(y->name);
    CHECK(y->state == symbol::DefinitionState::kMultiple);

    // Defining a local symbol doesn't affect the state of a symbol in another table.
    CHECK(z->state == symbol::DefinitionState::kUndefined);
  }

  SECTION("Export/import one global") {
    auto b1 = QSharedPointer<symbol::Table>::create();
    auto b2 = b1->addChild();
    auto l1 = b1->addChild();
    auto l2 = b2->addChild();
    auto l3 = b2->addChild();
    auto x = l1->reference("hello");
    auto y = l2->reference("hello");
    auto z = l3->reference("hello");

    l1->markGlobal("hello");
    CHECK(x->binding == symbol::Binding::kGlobal);
    CHECK(y->binding == symbol::Binding::kImported);
    CHECK(z->binding == symbol::Binding::kImported);
    CHECK(x->state == symbol::DefinitionState::kUndefined);
    CHECK(y->state == symbol::DefinitionState::kUndefined);
    CHECK(z->state == symbol::DefinitionState::kUndefined);
    // Check that defining a global symbol also defines its imports.
    l1->define("hello");
    CHECK(x->state == symbol::DefinitionState::kSingle);
    CHECK(y->state == symbol::DefinitionState::kSingle);
    CHECK(z->state == symbol::DefinitionState::kSingle);
    l2->define("hello");
    CHECK(x->state == symbol::DefinitionState::kSingle);
    CHECK(y->state == symbol::DefinitionState::kExternalMultiple);
    CHECK(z->state == symbol::DefinitionState::kSingle);
    l3->define("hello");
    CHECK(x->state == symbol::DefinitionState::kSingle);
    CHECK(y->state == symbol::DefinitionState::kExternalMultiple);
    CHECK(z->state == symbol::DefinitionState::kExternalMultiple);
  }

  SECTION("Multiple global definitions") {
    auto b1 = QSharedPointer<symbol::Table>::create();
    auto b2 = b1->addChild();
    auto l1 = b1->addChild();
    auto l2 = b2->addChild();
    auto l3 = b2->addChild();
    auto x = l1->reference("hello");
    auto y = l2->reference("hello");
    l1->markGlobal("hello");
    CHECK(x->binding == symbol::Binding::kGlobal);
    CHECK(y->binding == symbol::Binding::kImported);
    l2->markGlobal("hello");
    //  Test that reused name set to global gets treated as global too
    auto z = l3->reference("hello");
    CHECK(x->binding == symbol::Binding::kGlobal);
    CHECK(y->binding == symbol::Binding::kGlobal);
    CHECK(z->binding == symbol::Binding::kGlobal);
    CHECK(x->state == symbol::DefinitionState::kExternalMultiple);
    CHECK(y->state == symbol::DefinitionState::kExternalMultiple);
    CHECK(z->state == symbol::DefinitionState::kExternalMultiple);
  }

/*
 *  Test 2 Tree structures
 *          Branch1     Branch2
 *             |        |     |
 *           Leaf1    Leaf2 Leaf3
 */
  SECTION("Symbol existence checks with 2 independent trees.") {
    auto b1 = QSharedPointer<symbol::Table>::create();
    auto b2 = b1->addChild();
    auto l1 = b1->addChild();
    auto l2 = b2->addChild();
    auto l3 = b2->addChild();
    auto x = l1->reference("x");
    auto y = l2->reference("y");
    auto z = l3->reference("z");

    // Each leaf should find it's own children regardless of policy.
    CHECK(symbol::exists(l1, "x"));
    CHECK(symbol::exists(l2, "y", symbol::TraversalPolicy::kSiblings));
    CHECK(symbol::exists(l3, "z", symbol::TraversalPolicy::kWholeTree));

    // Check that traversal policy is respected.
    // Leaf 2 should not be able to see leaf 3's symbol with kChildren.
    // Leaf 1 cannot see leaf 2 or 3
    CHECK_FALSE(symbol::exists(l2, "z"));
    CHECK_FALSE(symbol::exists(l3, "y"));
    CHECK_FALSE(symbol::exists(l1, "z"));
    CHECK_FALSE(symbol::exists(l1, "y"));

    //  Lower leafs can see other siblings but not another tree with kSiblings
    CHECK(symbol::exists(l2, "z", symbol::TraversalPolicy::kSiblings));
    CHECK(symbol::exists(l3, "y", symbol::TraversalPolicy::kSiblings));
    CHECK_FALSE(symbol::exists(l2, "x"));
    CHECK_FALSE(symbol::exists(l3, "x"));

    //  Lower leafs cannot see other trees even with kWholeTree search
    CHECK_FALSE(symbol::exists(l2, "x", symbol::TraversalPolicy::kWholeTree));
    CHECK_FALSE(symbol::exists(l3, "x", symbol::TraversalPolicy::kWholeTree));
    CHECK_FALSE(symbol::exists(l1, "z", symbol::TraversalPolicy::kWholeTree));
    CHECK_FALSE(symbol::exists(l1, "y", symbol::TraversalPolicy::kWholeTree));
  }

  SECTION("Multiple global definitions with 2 independent trees.") {
    auto b1 = QSharedPointer<symbol::Table>::create();
    auto b2 = b1->addChild();
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

    //  Marking second branch as global does not trigger external multiple error in other branch
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
