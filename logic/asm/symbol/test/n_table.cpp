
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

#include "asm/symbol/table.hpp"
#include "asm/symbol/visit.hpp"
#include <QTest>

class SymbolNTable : public QObject {
  Q_OBJECT
private slots:
  /*
   *  Test Tree structure
   *          Branch (Parent)
   *          |             |
   *          Branch2     Leaf1
   *          |     |
   *        Leaf2 Leaf3
   */

  void independentReferences() {
    auto b1 = QSharedPointer<symbol::Table>::create(2);
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
    QCOMPARE_NE(x, y);
    QCOMPARE_NE(x, z);
    QCOMPARE_NE(y, z);
  }
  void findByName() {
    auto b1 = QSharedPointer<symbol::Table>::create(2);
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
    QCOMPARE(x1, x2);
    QCOMPARE(y1, y2);
    QCOMPARE(z1, z2);
    // Check that reference doesn't leak over.
    QCOMPARE_NE(x1, y1);
    QCOMPARE_NE(x2, z2);
    QCOMPARE_NE(y2, z1);
  }
  //  Dave: Added get tests
  void getByName() {
    auto b1 = QSharedPointer<symbol::Table>::create(2);
    auto b2 = b1->addChild();
    auto l1 = b1->addChild();
    auto l2 = b2->addChild();
    auto l3 = b2->addChild();
    auto x = l1->get("hello");
    QCOMPARE(x, std::nullopt);
    auto y = l2->get("hello");
    QCOMPARE(y, std::nullopt);
    auto z = l3->get("hello");
    QCOMPARE(z, std::nullopt);
    auto x1 = l1->reference("hello");
    auto x2 = l1->get("hello");
    QCOMPARE(x2, x1);
    auto y1 = l2->define("hello"); //  Uses define instead of reference
    auto y2 = l2->get("hello");
    QCOMPARE(y2, y1);
    auto z1 = l3->define("hello"); //  Uses define instead of reference
    auto z2 = l3->get("hello");
    QCOMPARE(z2, z2);
  }
  void existenceChecks() {
    auto b1 = QSharedPointer<symbol::Table>::create(2);
    auto b2 = b1->addChild();
    auto l1 = b1->addChild();
    auto l2 = b2->addChild();
    auto l3 = b2->addChild();
    auto x = l1->reference("x");
    auto y = l2->reference("y");
    auto z = l3->reference("z");

    // Each leaf should find it's own children regardless of policy.
    QVERIFY(symbol::exists(l1, "x"));
    QVERIFY(symbol::exists(l2, "y", symbol::TraversalPolicy::kSiblings));
    QVERIFY(symbol::exists(l3, "z", symbol::TraversalPolicy::kWholeTree));

    // Check that traversal policy is respected.
    // Leaf 2 should not be able to see leaf 3's symbol with kChildren.
    // Parent leaf can see all others
    QVERIFY(!symbol::exists(l2, "z"));
    QVERIFY(!symbol::exists(l3, "y"));
    QVERIFY(!symbol::exists(l1, "z"));
    QVERIFY(!symbol::exists(l1, "y"));

    //  Lower leafs can see other siblings but not parent with kSiblings
    QVERIFY(symbol::exists(l2, "z", symbol::TraversalPolicy::kSiblings));
    QVERIFY(symbol::exists(l3, "y", symbol::TraversalPolicy::kSiblings));
    QVERIFY(!symbol::exists(l2, "x", symbol::TraversalPolicy::kSiblings));
    QVERIFY(!symbol::exists(l3, "x", symbol::TraversalPolicy::kSiblings));

    //  Lower leafs can see parent and siblings
    QVERIFY(symbol::exists(l2, "x", symbol::TraversalPolicy::kWholeTree));
    QVERIFY(symbol::exists(l3, "x", symbol::TraversalPolicy::kWholeTree));
    QVERIFY(symbol::exists(l1, "z", symbol::TraversalPolicy::kWholeTree));
    QVERIFY(symbol::exists(l1, "y", symbol::TraversalPolicy::kWholeTree));
  }
  void localityOfDefines() {
    auto b1 = QSharedPointer<symbol::Table>::create(2);
    auto b2 = b1->addChild();
    auto l1 = b1->addChild();
    auto l2 = b2->addChild();
    auto l3 = b2->addChild();
    auto x = l1->reference("hello");
    auto y = l2->define("hello");
    auto z = l3->reference("hello");

    QCOMPARE(x->state, symbol::DefinitionState::kUndefined);
    l1->define(x->name);
    QCOMPARE(x->state, symbol::DefinitionState::kSingle);
    l1->define(x->name);
    QCOMPARE(x->state, symbol::DefinitionState::kMultiple);
    // Y was created and defined in one step. Check state
    QCOMPARE(y->state, symbol::DefinitionState::kSingle);
    l2->define(y->name);
    QCOMPARE(y->state, symbol::DefinitionState::kMultiple);

    // Defining a local symbol doesn't affect the state of a symbol in another
    // table.
    QCOMPARE(z->state, symbol::DefinitionState::kUndefined);
  }
  void singleValidGlobal() {
    auto b1 = QSharedPointer<symbol::Table>::create(2);
    auto b2 = b1->addChild();
    auto l1 = b1->addChild();
    auto l2 = b2->addChild();
    auto l3 = b2->addChild();
    auto x = l1->reference("hello");
    auto y = l2->reference("hello");
    auto z = l3->reference("hello");

    l1->markGlobal("hello");
    QCOMPARE(x->binding, symbol::Binding::kGlobal);
    QCOMPARE(y->binding, symbol::Binding::kImported);
    QCOMPARE(z->binding, symbol::Binding::kImported);
    QCOMPARE(x->state, symbol::DefinitionState::kUndefined);
    QCOMPARE(y->state, symbol::DefinitionState::kUndefined);
    QCOMPARE(z->state, symbol::DefinitionState::kUndefined);
    // Check that defining a global symbol also defines its imports.
    l1->define("hello");
    QCOMPARE(x->state, symbol::DefinitionState::kSingle);
    QCOMPARE(y->state, symbol::DefinitionState::kSingle);
    QCOMPARE(z->state, symbol::DefinitionState::kSingle);
    l2->define("hello");
    QCOMPARE(x->state, symbol::DefinitionState::kSingle);
    QCOMPARE(y->state, symbol::DefinitionState::kExternalMultiple);
    QCOMPARE(z->state, symbol::DefinitionState::kSingle);
    l3->define("hello");
    QCOMPARE(x->state, symbol::DefinitionState::kSingle);
    QCOMPARE(y->state, symbol::DefinitionState::kExternalMultiple);
    QCOMPARE(z->state, symbol::DefinitionState::kExternalMultiple);
  }
  void multipleGlobal() {
    auto b1 = QSharedPointer<symbol::Table>::create(2);
    auto b2 = b1->addChild();
    auto l1 = b1->addChild();
    auto l2 = b2->addChild();
    auto l3 = b2->addChild();
    auto x = l1->reference("hello");
    auto y = l2->reference("hello");
    l1->markGlobal("hello");
    QCOMPARE(x->binding, symbol::Binding::kGlobal);
    QCOMPARE(y->binding, symbol::Binding::kImported);
    l2->markGlobal("hello");
    //  Test that reused name set to global gets treated as global too
    auto z = l3->reference("hello");
    QCOMPARE(x->binding, symbol::Binding::kGlobal);
    QCOMPARE(y->binding, symbol::Binding::kGlobal);
    QCOMPARE(z->binding, symbol::Binding::kGlobal);
    QCOMPARE(x->state, symbol::DefinitionState::kExternalMultiple);
    QCOMPARE(y->state, symbol::DefinitionState::kExternalMultiple);
    QCOMPARE(z->state, symbol::DefinitionState::kExternalMultiple);
  }
};

#include "n_table.moc"

QTEST_MAIN(SymbolNTable);
