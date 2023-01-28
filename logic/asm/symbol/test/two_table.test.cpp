#include "catch.hpp"

#include "symbol/table.hpp"
#include "symbol/types.hpp"
#include "symbol/value.hpp"
#include "symbol/visit.hpp"
TEST_CASE("Validate functionality for 2 symbol table.") {

  SECTION("Check that local references are independent.") {
    auto st = QSharedPointer<symbol::Table>::create();
    auto st1 = st->addChild();
    auto st2 = st->addChild();
    auto x = st1->reference("hello");
    auto y = st2->reference("hello");
    // 2. 1 for local copy, 1 in map.
    // CHECK(x.use_count() == 2);
    CHECK(x != y);
  }

  SECTION("Find by name.") {
    auto st = QSharedPointer<symbol::Table>::create();
    auto st1 = st->addChild();
    auto st2 = st->addChild();
    auto x = st1->reference("hello");
    auto y = st1->reference("hello");
    auto z = st2->reference("hello");
    CHECK(x == y);
    // Check that reference doesn't leak over.
    CHECK(z != x);
    CHECK(z != y);
  }

    //  Dave: Added get tests
  SECTION("Get by name using reference.") {
    auto st = QSharedPointer<symbol::Table>::create();
    auto st1 = st->addChild();
    auto st2 = st->addChild();
    auto x = st1->get("hello");
    CHECK(x == std::nullopt);
    auto y = st1->get("hello");
    CHECK(y == std::nullopt);
    auto x1 = st1->reference("hello");
    auto x2 = st1->get("hello");
    CHECK(x1 == x2);
    auto y1 = st2->define("hello");   //  Uses define instead of reference
    auto y2 = st2->get("hello");
    CHECK(y1 == y2);
    CHECK(x1 != y1);
  }

  SECTION("Symbol existence checks.") {
    auto st = QSharedPointer<symbol::Table>::create();
    auto st1 = st->addChild();
    auto st2 = st->addChild();
    // Discard reference returned by st.
    st1->reference("hello");
    // Check that traversal policy is respected.
    // Table 2 should not be able to see table 1's symbol with kChildren.
    CHECK_FALSE(symbol::exists(st2, "hello"));
    // But it can see them when it is allowed to check siblings.
    CHECK(symbol::exists(st2, "hello", symbol::TraversalPolicy::kSiblings));
    // Trivially the root can always see any symbol.
    CHECK(symbol::exists(st, "hello"));
  }

    //  Dave: Check symbol directly against pointer
  SECTION("Symbol existence checks.") {
    auto st = QSharedPointer<symbol::Table>::create();
    auto st1 = st->addChild();
    auto st2 = st->addChild();
    st1->reference("hello");
    // Check that traversal policy is respected.
    // This calls exists from table directly
    CHECK(st1->exists("hello"));
    CHECK_FALSE(st2->exists("hello"));
  }

  SECTION("define() a local in one table does not affect the other.") {
    auto st = QSharedPointer<symbol::Table>::create();
    auto st1 = st->addChild();
    auto st2 = st->addChild();
    auto x = st1->reference("hello");
    auto y = st2->reference("hello");
    CHECK(x->state == symbol::DefinitionState::kUndefined);
    st1->define(x->name);
    CHECK(x->state == symbol::DefinitionState::kSingle);
    st1->define(x->name);
    CHECK(x->state == symbol::DefinitionState::kMultiple);
    // Defining a local symbol doesn't affect the state of a symbol in another table.
    CHECK(y->state == symbol::DefinitionState::kUndefined);
  }

  SECTION("Export/import one global") {
    auto st = QSharedPointer<symbol::Table>::create();
    auto st1 = st->addChild();
    auto st2 = st->addChild();
    auto x = st1->reference("hello");
    auto y = st2->reference("hello");
    st1->markGlobal("hello");
    CHECK(x->binding == symbol::Binding::kGlobal);
    CHECK(y->binding == symbol::Binding::kImported);
    CHECK(x->state == symbol::DefinitionState::kUndefined);
    CHECK(y->state == symbol::DefinitionState::kUndefined);
    // Check that defining a global symbol also defines its imports.
    st1->define("hello");
    CHECK(x->state == symbol::DefinitionState::kSingle);
    CHECK(y->state == symbol::DefinitionState::kSingle);
    st2->define("hello");
    CHECK(x->state == symbol::DefinitionState::kSingle);
    CHECK(y->state == symbol::DefinitionState::kExternalMultiple);
  }

  SECTION("Multiple global definitions") {
    auto st = QSharedPointer<symbol::Table>::create();
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
}
