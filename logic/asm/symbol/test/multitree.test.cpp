#include "symbol/table.hpp"
#include "symbol/visit.hpp"
#include <QTest>
class SymbolMultiTree : public QObject {
  Q_OBJECT
private slots:
  /*
   *  Test 2 Tree structures
   *          Branch1     Branch2
   *             |        |     |
   *           Leaf1    Leaf2 Leaf3
   */
  void independentExistenceQCOMPAREs() {
    auto b1 = QSharedPointer<symbol::Table>::create();
    auto b2 = QSharedPointer<symbol::Table>::create();
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

    // QCOMPARE that traversal policy is respected.
    // Leaf 2 should not be able to see leaf 3's symbol with kChildren.
    // Leaf 1 cannot see leaf 2 or 3
    QVERIFY(!symbol::exists(l2, "z"));
    QVERIFY(!symbol::exists(l3, "y"));
    QVERIFY(!symbol::exists(l1, "z"));
    QVERIFY(!symbol::exists(l1, "y"));

    //  Lower leafs can see other siblings but not another tree with kSiblings
    QVERIFY(symbol::exists(l2, "z", symbol::TraversalPolicy::kSiblings));
    QVERIFY(symbol::exists(l3, "y", symbol::TraversalPolicy::kSiblings));
    QVERIFY(!symbol::exists(l2, "x"));
    QVERIFY(!symbol::exists(l3, "x"));

    //  Lower leafs cannot see other trees even with kWholeTree search
    QVERIFY(!symbol::exists(l2, "x", symbol::TraversalPolicy::kWholeTree));
    QVERIFY(!symbol::exists(l3, "x", symbol::TraversalPolicy::kWholeTree));
    QVERIFY(!symbol::exists(l1, "z", symbol::TraversalPolicy::kWholeTree));
    QVERIFY(!symbol::exists(l1, "y", symbol::TraversalPolicy::kWholeTree));
  }
  void independentGlobals() {
    auto b1 = QSharedPointer<symbol::Table>::create();
    auto b2 = QSharedPointer<symbol::Table>::create();
    auto l1 = b1->addChild();
    auto l2 = b2->addChild();
    auto l3 = b2->addChild();
    auto x = l1->reference("hello");
    auto y = l2->reference("hello");

    //  Marking branch leaf 1 as global does not impact branch 2 leaves
    l1->markGlobal("hello");
    QCOMPARE(x->binding, symbol::Binding::kGlobal);
    QCOMPARE(y->binding, symbol::Binding::kLocal);
    QCOMPARE(x->state, symbol::DefinitionState::kUndefined);
    QCOMPARE(y->state, symbol::DefinitionState::kUndefined);

    //  Marking second branch as global does not trigger external multiple error
    //  in other branch
    l2->markGlobal("hello");
    QCOMPARE(x->binding, symbol::Binding::kGlobal);
    QCOMPARE(y->binding, symbol::Binding::kGlobal);
    QCOMPARE(x->state, symbol::DefinitionState::kUndefined);
    QCOMPARE(y->state, symbol::DefinitionState::kUndefined);

    //  Trigger global clash in branch 2
    auto z = l3->reference("hello");
    QCOMPARE(z->binding, symbol::Binding::kImported);
    QCOMPARE(z->state, symbol::DefinitionState::kUndefined);

    //  Create global error with second leaf value marked as global
    l3->markGlobal("hello");
    QCOMPARE(z->binding, symbol::Binding::kGlobal);
    QCOMPARE(x->state, symbol::DefinitionState::kUndefined);
    QCOMPARE(y->state, symbol::DefinitionState::kExternalMultiple);
    QCOMPARE(z->state, symbol::DefinitionState::kExternalMultiple);
  }
};

#include "multitree.test.moc"

QTEST_MAIN(SymbolMultiTree)
