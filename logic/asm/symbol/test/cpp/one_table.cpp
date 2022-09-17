#include "catch.hpp"

#include "symbol/table.hpp"
#include "symbol/types.hpp"
#include "symbol/value.hpp"
TEST_CASE("Validate functionality for 1 symbol table.") {

    SECTION("Add symbol via reference(std::string).") {
        auto st = std::make_shared<symbol::LeafTable<uint16_t>>();
        auto x = st->reference("hello");
        // 2. 1 for local copy, 1 in map.
        CHECK(x.use_count() == 2);
    }

    SECTION("Find by name.") {
        auto st = std::make_shared<symbol::LeafTable<uint16_t>>();
        auto x = st->reference("hello");
        auto y = st->reference("hello");
        CHECK(x == y);
    }

    SECTION("Require case matching.") {
        auto st = std::make_shared<symbol::LeafTable<uint16_t>>();
        auto x = st->reference("hello");
        auto y = st->reference("Hello");
        CHECK_FALSE(x == y);
    }

    SECTION("Symbol existence checks.") {
        auto st = std::make_shared<symbol::LeafTable<uint16_t>>();
        // Discard reference returned by st.
        st->reference("hello");
        CHECK_FALSE(symbol::exists<uint16_t>({st}, "bye"));
        CHECK_FALSE(symbol::exists<uint16_t>({st}, "Hello"));
        CHECK(symbol::exists<uint16_t>({st}, "hello"));
    }

    SECTION("Undefined => defined => multiply defined") {
        auto st = std::make_shared<symbol::LeafTable<uint16_t>>();
        auto x = st->reference("hello");
        CHECK(x->state == symbol::definition_state::kUndefined);
        st->define(x->name);
        CHECK(x->state == symbol::definition_state::kSingle);
        st->define(x->name);
        CHECK(x->state == symbol::definition_state::kMultiple);
    }

    SECTION("Define before reference.") {
        auto st = std::make_shared<symbol::LeafTable<uint16_t>>();
        st->define("hello");
        auto x = st->reference("hello");
        CHECK(x->state == symbol::definition_state::kSingle);
    }

    for (int it = 0; it < 4; it++) {
        DYNAMIC_SECTION("Undefined count == " << it) {
            auto st = std::make_shared<symbol::LeafTable<uint16_t>>();
            for (int i = 0; i < it; i++)
                st->reference("hello");
            CHECK(st->reference("hello")->state == symbol::definition_state::kUndefined);
        }
        if (it < 2)
            continue;
        DYNAMIC_SECTION("Multiply defined count == " << it) {
            auto st = std::make_shared<symbol::LeafTable<uint16_t>>();
            for (int i = 0; i < it; i++)
                st->define("hello");
            CHECK(st->reference("hello")->state == symbol::definition_state::kMultiple);
        }
    }

    SECTION("Offset modification, all.") {
        auto st = std::make_shared<symbol::LeafTable<uint16_t>>();
        auto x0 = st->define("h0");
        auto x1 = st->define("h1");
        auto x2 = st->define("h2");

        x0->value = std::make_shared<symbol::value_location<uint16_t>>(10, 0, symbol::Type::kObject);
        x1->value = std::make_shared<symbol::value_location<uint16_t>>(20, 0, symbol::Type::kObject);
        x2->value = std::make_shared<symbol::value_location<uint16_t>>(30, 0, symbol::Type::kCode);
        symbol::adjust_offset<uint16_t>(st, 1, 10);

        CHECK(x0->value->value() == 11);
        CHECK(x1->value->value() == 21);
        CHECK(x2->value->value() == 31);
        // Test that listing doesn't crash or fail to compile.
        std:: cout << symbol::symbol_table_listing<uint16_t>(st) << std::endl;
    }

    SECTION("Offset modification, threshold.") {
        auto st = std::make_shared<symbol::LeafTable<uint16_t>>();
        auto x0 = st->define("h0");
        auto x1 = st->define("h1");
        auto x2 = st->define("h2");

        x0->value = std::make_shared<symbol::value_location<uint16_t>>(10, 0, symbol::Type::kObject);
        x1->value = std::make_shared<symbol::value_location<uint16_t>>(20, 0, symbol::Type::kObject);
        x2->value = std::make_shared<symbol::value_location<uint16_t>>(30, 0, symbol::Type::kObject);
        symbol::adjust_offset<uint16_t>(st, -1, 15);

        CHECK(x0->value->value() == 10);
        CHECK(x1->value->value() == 19);
        CHECK(x2->value->value() == 29);
    }
    SECTION("Redundant mark as global") {
        auto st = std::make_shared<symbol::LeafTable<uint16_t>>();
        auto x = st->reference("hello");
        st->mark_global("hello");
        st->mark_global("hello");
        CHECK(x->binding == symbol::binding_t::kGlobal);
        CHECK(x->state == symbol::definition_state::kUndefined);
    }
}