/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
 *
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

#include "symbol/table.hpp"
#include "symbol/types.hpp"
#include "symbol/value.hpp"
#include "symbol/visit.hpp"
#include <QObject>
#include <QTest>
class Symbol1Table : public QObject {
  Q_OBJECT
private slots:
  void findByName() {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto x = st->reference("hello");
    auto y = st->reference("hello");
    QCOMPARE(x, y);
  }
  //  Dave: Added get tests
  void getByName() {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto z = st->get("hello");
    QCOMPARE(z, std::nullopt);
    auto x = st->reference("hello");
    auto y = st->get("hello");
    QCOMPARE(x, y);
  }
  void getByReferences() {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto x = st->reference("hello");
    auto y = st->reference("Hello");
    QCOMPARE_NE(x, y);
  }
  void caseSensitive() {
    auto st = QSharedPointer<symbol::Table>::create(2);
    // Discard reference returned by st.
    st->reference("hello");
    //  Uses visitor pattern in visit.hpp
    QVERIFY(!symbol::exists(st, "bye"));
    QVERIFY(!symbol::exists(st, "Hello"));
    QVERIFY(symbol::exists(st, "hello"));
  }
  void existenceChecksTable() {
    auto st = QSharedPointer<symbol::Table>::create(2);
    st->reference("hello");
    //  Uses table specific exists function
    QVERIFY(!st->exists("bye"));
    QVERIFY(!st->exists("Hello"));
    QVERIFY(st->exists("hello"));
  }
  void existenceChecksFunction() {
    auto st = QSharedPointer<symbol::Table>::create(2);
    // Discard reference returned by st.
    st->reference("hello");
    //  Uses visitor pattern in visit.hpp
    QVERIFY(!symbol::exists(st, "bye"));
    QVERIFY(!symbol::exists(st, "Hello"));
    QVERIFY(symbol::exists(st, "hello"));
  }
  void progressThroughDefinitionStates() {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto x = st->reference("hello");
    QCOMPARE(x->state, symbol::DefinitionState::kUndefined);
    st->define(x->name);
    QCOMPARE(x->state, symbol::DefinitionState::kSingle);
    // CHECK(x.use_count() == 2);
    st->define(x->name);
    QCOMPARE(x->state, symbol::DefinitionState::kMultiple);
    // CHECK(x.use_count() == 2);
  }
  void defineBeforeReference() {
    auto st = QSharedPointer<symbol::Table>::create(2);
    st->define("hello");
    auto x = st->reference("hello");
    QCOMPARE(x->state, symbol::DefinitionState::kSingle);
  }
  // Yes, the symbols are multiple-defined, but need to ensure they point to
  // same symbol.
  //  Dave: Test that define works like Reference
  void findByNameDefine() {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto x = st->define("hello");
    auto y = st->define("hello");
    QCOMPARE(x->state, symbol::DefinitionState::kMultiple);
    QCOMPARE(x, y);
  }
  void defineAndGet() {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto z = st->get("hello");
    QCOMPARE(z, std::nullopt);
    auto x = st->define("hello");
    auto y = st->get("hello");
    QCOMPARE(x, y);
  }
  void defineCaseSensitive() {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto x = st->define("hello");
    auto y = st->define("Hello");
    QCOMPARE_NE(x, y);
  }
  void defineAndExists() {
    auto st = QSharedPointer<symbol::Table>::create(2);
    st->define("hello");
    QVERIFY(!st->exists("bye"));
    QVERIFY(!st->exists("Hello"));
    QVERIFY(st->exists("hello"));
  }
  void multiReference() {
    for (int it = 0; it < 4; it++) {
      auto st = QSharedPointer<symbol::Table>::create(2);
      for (int i = 0; i < it; i++)
        st->reference("hello");
      QCOMPARE(st->reference("hello")->state,
               symbol::DefinitionState::kUndefined);
    }
  }
  void multiDefines() {
    for (int it = 2; it < 4; it++) {
      auto st = QSharedPointer<symbol::Table>::create(2);
      for (int i = 0; i < it; i++)
        st->define("hello");
      QCOMPARE(st->reference("hello")->state,
               symbol::DefinitionState::kMultiple);
    }
  }
  void allOffsetModification() {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto x0 = st->define("h0");
    auto x1 = st->define("h1");
    auto x2 = st->define("h2");

    x0->value = QSharedPointer<symbol::value::Location>::create(
        2, 2, 10, 0, symbol::Type::kObject);
    x1->value = QSharedPointer<symbol::value::Location>::create(
        2, 2, 20, 0, symbol::Type::kObject);
    x2->value = QSharedPointer<symbol::value::Location>::create(
        2, 2, 30, 0, symbol::Type::kCode);
    symbol::adjustOffset(st, 1, 10);

    QCOMPARE(x0->value->value()(), 11);
    QCOMPARE(x1->value->value()(), 21);
    QCOMPARE(x2->value->value()(), 31);
  }
  void listingNoThrow() {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto x0 = st->define("h0");
    auto x1 = st->define("h1");
    auto x2 = st->define("h2");
    QVERIFY_THROWS_NO_EXCEPTION(symbol::tableListing(st, 2));
  }
  void thresholdOffsetModification() {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto x0 = st->define("h0");
    auto x1 = st->define("h1");
    auto x2 = st->define("h2");

    x0->value = QSharedPointer<symbol::value::Location>::create(
        2, 2, 10, 0, symbol::Type::kObject);
    x1->value = QSharedPointer<symbol::value::Location>::create(
        2, 2, 20, 0, symbol::Type::kObject);
    x2->value = QSharedPointer<symbol::value::Location>::create(
        2, 2, 30, 0, symbol::Type::kCode);
    symbol::adjustOffset(st, -1, 12);

    QCOMPARE(x0->value->value()(), 10);
    QCOMPARE(x1->value->value()(), 19);
    QCOMPARE(x2->value->value()(), 29);
  }
  void redundantMarkGlobal() {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto x = st->reference("hello");
    st->markGlobal("hello");
    st->markGlobal("hello");
    QCOMPARE(x->binding, symbol::Binding::kGlobal);
    QCOMPARE(x->state, symbol::DefinitionState::kUndefined);
  }
  void globalwithDefine() {
    auto st = QSharedPointer<symbol::Table>::create(2);
    auto x = st->define("hello");
    st->markGlobal("hello");
    st->markGlobal("hello"); //  Ignored
    QCOMPARE(x->binding, symbol::Binding::kGlobal);
    QCOMPARE(x->state, symbol::DefinitionState::kSingle);
  }
};
#include "one_table.moc"
QTEST_MAIN(Symbol1Table)
