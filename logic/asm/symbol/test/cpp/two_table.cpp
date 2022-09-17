#include "catch.hpp"

#include "symbol/table.hpp"
#include "symbol/value.hpp"
TEST_CASE("Validate functionality for 2 symbol table.") {

    SECTION("Check that local references are independent.") {
        auto st = std::make_shared<symbol::BranchTable<uint16_t>>();
        auto st1 = symbol::insert_leaf<uint16_t>(st);
        auto st2 = symbol::insert_leaf<uint16_t>(st);
        auto x = st1->reference("hello");
        auto y = st2->reference("hello");
        // 2. 1 for local copy, 1 in map.
        CHECK(x.use_count() == 2);
        CHECK(x != y);
    }

    SECTION("Find by name.") {
        auto st = std::make_shared<symbol::BranchTable<uint16_t>>();
        auto st1 = symbol::insert_leaf<uint16_t>(st);
        auto st2 = symbol::insert_leaf<uint16_t>(st);
        auto x = st1->reference("hello");
        auto y = st1->reference("hello");
        auto z = st2->reference("hello");
        CHECK(x == y);
        // Check that reference doesn't leak over.
        CHECK(z != x);
        CHECK(z != y);
    }

    SECTION("Symbol existence checks.") {
        auto st = std::make_shared<symbol::BranchTable<uint16_t>>();
        auto st1 = symbol::insert_leaf<uint16_t>(st);
        auto st2 = symbol::insert_leaf<uint16_t>(st);
        // Discard reference returned by st.
        st1->reference("hello");
        // Check that traversal policy is respected.
        // Table 2 should not be able to see table 1's symbol with kChildren.
        CHECK_FALSE(symbol::exists<uint16_t>({st2}, "hello"));
        // But it can see them when it is allowed to check siblings.
        CHECK(symbol::exists<uint16_t>({st2}, "hello", symbol::TraversalPolicy::kSiblings));
        // Trivially the root can always see any symbol.
        CHECK(symbol::exists<uint16_t>({st}, "hello"));
    }

    SECTION("define() a local in one table does not affect the other.") {
        auto st = std::make_shared<symbol::BranchTable<uint16_t>>();
        auto st1 = symbol::insert_leaf<uint16_t>(st);
        auto st2 = symbol::insert_leaf<uint16_t>(st);
        auto x = st1->reference("hello");
        auto y = st2->reference("hello");
        CHECK(x->state == symbol::definition_state::kUndefined);
        st1->define(x->name);
        CHECK(x->state == symbol::definition_state::kSingle);
        st1->define(x->name);
        CHECK(x->state == symbol::definition_state::kMultiple);
        // Defining a local symbol doesn't affect the state of a symbol in another table.
        CHECK(y->state == symbol::definition_state::kUndefined);
    }

    SECTION("Export/import one global") {
        auto st = std::make_shared<symbol::BranchTable<uint16_t>>();
        auto st1 = symbol::insert_leaf<uint16_t>(st);
        auto st2 = symbol::insert_leaf<uint16_t>(st);
        auto x = st1->reference("hello");
        auto y = st2->reference("hello");
        st1->mark_global("hello");
        CHECK(x->binding == symbol::binding_t::kGlobal);
        CHECK(y->binding == symbol::binding_t::kImported);
        CHECK(x->state == symbol::definition_state::kUndefined);
        CHECK(y->state == symbol::definition_state::kUndefined);
        // Check that defining a global symbol also defines its imports.
        st1->define("hello");
        CHECK(x->state == symbol::definition_state::kSingle);
        CHECK(y->state == symbol::definition_state::kSingle);
        st2->define("hello");
        CHECK(x->state == symbol::definition_state::kSingle);
        CHECK(y->state == symbol::definition_state::kExternalMultiple);
    }

    SECTION("Multiple global definitions") {
        auto st = std::make_shared<symbol::BranchTable<uint16_t>>();
        auto st1 = symbol::insert_leaf<uint16_t>(st);
        auto st2 = symbol::insert_leaf<uint16_t>(st);
        auto x = st1->reference("hello");
        auto y = st2->reference("hello");
        st1->mark_global("hello");
        st2->mark_global("hello");
        CHECK(x->binding == symbol::binding_t::kGlobal);
        CHECK(y->binding == symbol::binding_t::kGlobal);
        CHECK(x->state == symbol::definition_state::kExternalMultiple);
        CHECK(y->state == symbol::definition_state::kExternalMultiple);
    }
}