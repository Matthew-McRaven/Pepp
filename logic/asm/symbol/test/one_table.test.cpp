#include "catch.hpp"

#include "symbol/table.hpp"
#include "symbol/types.hpp"
#include "symbol/value.hpp"
#include "symbol/visit.hpp"
TEST_CASE("Validate functionality for 1 symbol table.") {

  /*SECTION("Add symbol via reference(std::string).") {
      auto st = std::make_shared<symbol::LeafTable<uint16_t>>();
      auto x = st->reference("hello");
      // 2. 1 for local copy, 1 in map.
      // CHECK(x.use_count() == 2);
  }*/

  SECTION("Find by name.") {
    auto st = QSharedPointer<symbol::Table>::create();
    auto x = st->reference("hello");
    auto y = st->reference("hello");
    CHECK(x == y);
  }

  //  Dave: Added get tests
  SECTION("Get by name using reference.") {
    auto st = QSharedPointer<symbol::Table>::create();
    auto z = st->get("hello");
    CHECK(z == std::nullopt);
    auto x = st->reference("hello");
    auto y = st->get("hello");
    CHECK(x == y);
  }

  SECTION("Require case matching.") {
    auto st = QSharedPointer<symbol::Table>::create();
    auto x = st->reference("hello");
    auto y = st->reference("Hello");
    CHECK_FALSE(x == y);
  }

  SECTION("Symbol existence checks.") {
    auto st = QSharedPointer<symbol::Table>::create();
    // Discard reference returned by st.
    st->reference("hello");
    //  Uses visitor pattern in visit.hpp
    CHECK_FALSE(symbol::exists(st, "bye"));
    CHECK_FALSE(symbol::exists(st, "Hello"));
    CHECK(symbol::exists(st, "hello"));
  }

  //  Dave: Check symbol directly against pointer
  SECTION("Symbol existence checks with pointer.") {
    auto st = QSharedPointer<symbol::Table>::create();
    st->reference("hello");
    //  Uses table specific exists function
    CHECK_FALSE(st->exists("bye"));
    CHECK_FALSE(st->exists("Hello"));
    CHECK(st->exists("hello"));
  }

  SECTION("Undefined => defined => multiply defined") {
    auto st = QSharedPointer<symbol::Table>::create();
    auto x = st->reference("hello");
    CHECK(x->state == symbol::DefinitionState::kUndefined);
    st->define(x->name);
    CHECK(x->state == symbol::DefinitionState::kSingle);
    // CHECK(x.use_count() == 2);
    st->define(x->name);
    CHECK(x->state == symbol::DefinitionState::kMultiple);
    // CHECK(x.use_count() == 2);
  }

  SECTION("Define before reference.") {
    auto st = QSharedPointer<symbol::Table>::create();
    st->define("hello");
    auto x = st->reference("hello");
    CHECK(x->state == symbol::DefinitionState::kSingle);
  }

  //  Dave: Test that define works like Reference
  /*SECTION("Add symbol via define(std::string).") {
    auto st = std::make_shared<symbol::LeafTable<uint16_t>>();
    auto x = st->define("hello");
    // 2. 1 for local copy, 1 in map.
    // CHECK(x.use_count() == 2);
  }*/

  //  Dave: Test that define works like Reference
  SECTION("Find by name using Define.") {
    auto st = QSharedPointer<symbol::Table>::create();
    auto x = st->define("hello");
    auto y = st->define("hello");
    CHECK(x->state == symbol::DefinitionState::kMultiple);
    CHECK(x == y);
  }

  //  Dave: Test that define works like Reference
  //  Dave: Added get tests
  SECTION("Get by name using define.") {
    auto st = QSharedPointer<symbol::Table>::create();
    auto z = st->get("hello");
    CHECK(z == std::nullopt);
    auto x = st->define("hello");
    auto y = st->get("hello");
    CHECK(x == y);
  }

  //  Dave: Test that define works like Reference
  SECTION("Require case matching using define.") {
    auto st = QSharedPointer<symbol::Table>::create();
    auto x = st->define("hello");
    auto y = st->define("Hello");
    CHECK_FALSE(x == y);
  }

  //  Dave: Test that define works like Reference
  //  Dave: Check symbol directly against pointer
  SECTION("Symbol existence checks with pointer.") {
    auto st = QSharedPointer<symbol::Table>::create();
    st->define("hello");
    CHECK_FALSE(st->exists("bye"));
    CHECK_FALSE(st->exists("Hello"));
    CHECK(st->exists("hello"));
  }

  for (int it = 0; it < 4; it++) {
    DYNAMIC_SECTION("Undefined count == " << it) {
      auto st = QSharedPointer<symbol::Table>::create();
      for (int i = 0; i < it; i++)
        st->reference("hello");
      CHECK(st->reference("hello")->state ==
            symbol::DefinitionState::kUndefined);
    }
    if (it < 2)
      continue;
    DYNAMIC_SECTION("Multiply defined count == " << it) {
      auto st = QSharedPointer<symbol::Table>::create();
      for (int i = 0; i < it; i++)
        st->define("hello");
      CHECK(st->reference("hello")->state ==
            symbol::DefinitionState::kMultiple);
    }
  }

  SECTION("Offset modification, all.") {
    auto st = QSharedPointer<symbol::Table>::create();
    auto x0 = st->define("h0");
    auto x1 = st->define("h1");
    auto x2 = st->define("h2");

    x0->value = QSharedPointer<symbol::value::Location>::create(
        2, 10, 0, symbol::Type::kObject);
    x1->value = QSharedPointer<symbol::value::Location>::create(
        2, 20, 0, symbol::Type::kObject);
    x2->value = QSharedPointer<symbol::value::Location>::create(
        2, 30, 0, symbol::Type::kCode);
    symbol::adjustOffset(st, 1, 10);

    CHECK(x0->value->value()() == 11);
    CHECK(x1->value->value()() == 21);
    CHECK(x2->value->value()() == 31);
    // Test that listing doesn't crash or fail to compile.

    QTextStream(stdout) << symbol::tableListing(st, 2) << "\n";
  }

  SECTION("Offset modification, threshold.") {
    auto st = QSharedPointer<symbol::Table>::create();
    auto x0 = st->define("h0");
    auto x1 = st->define("h1");
    auto x2 = st->define("h2");

    x0->value = QSharedPointer<symbol::value::Location>::create(
        2, 10, 0, symbol::Type::kObject);
    x1->value = QSharedPointer<symbol::value::Location>::create(
        2, 20, 0, symbol::Type::kObject);
    x2->value = QSharedPointer<symbol::value::Location>::create(
        2, 30, 0, symbol::Type::kObject);
    symbol::adjustOffset(st, -1, 15);

    CHECK(x0->value->value()() == 10);
    CHECK(x1->value->value()() == 19);
    CHECK(x2->value->value()() == 29);
  }

  SECTION("Redundant mark as global") {
    auto st = QSharedPointer<symbol::Table>::create();
    auto x = st->reference("hello");
    st->mark_global("hello");
    st->mark_global("hello");
    CHECK(x->binding == symbol::Binding::kGlobal);
    CHECK(x->state == symbol::DefinitionState::kUndefined);
  }

  //  Dave: Test that define works like Reference
  SECTION("Redundant mark as global using define") {
    auto st = QSharedPointer<symbol::Table>::create();
    auto x = st->define("hello");
    st->mark_global("hello");
    st->mark_global("hello"); //  Ignored
    CHECK(x->binding == symbol::Binding::kGlobal);
    CHECK(x->state == symbol::DefinitionState::kSingle);
  }
}
