
#include "symbol/table.hpp"
#include "symbol/types.hpp"
#include "symbol/visit.hpp"
#include <QObject>
#include <QTest>

class Symbol2Table : public QObject {
  Q_OBJECT
private slots:
  void independentReferences() {
    auto st = QSharedPointer<symbol::Table>::create();
    auto st1 = st->addChild();
    auto st2 = st->addChild();
    auto x = st1->reference("hello");
    auto y = st2->reference("hello");
    // 2. 1 for local copy, 1 in map.
    // QCOMPARE(x.use_count() == 2);
    QCOMPARE_NE(x, y);
  }
  void findByName() {
    auto st = QSharedPointer<symbol::Table>::create();
    auto st1 = st->addChild();
    auto st2 = st->addChild();
    auto x = st1->reference("hello");
    auto y = st1->reference("hello");
    auto z = st2->reference("hello");
    QCOMPARE(x, y);
    // QCOMPARE that reference doesn't leak over.
    QCOMPARE_NE(z, x);
    QCOMPARE_NE(z, y);
  }
  //  Dave: Added get tests
  void getByName() {
    auto st = QSharedPointer<symbol::Table>::create();
    auto st1 = st->addChild();
    auto st2 = st->addChild();
    auto x = st1->get("hello");
    QCOMPARE(x, std::nullopt);
    auto y = st1->get("hello");
    QCOMPARE(y, std::nullopt);
    auto x1 = st1->reference("hello");
    auto x2 = st1->get("hello");
    QCOMPARE(x1, x2);
    auto y1 = st2->define("hello"); //  Uses define instead of reference
    auto y2 = st2->get("hello");
    QCOMPARE(y1, y2);
    QCOMPARE_NE(x1, y1);
  }
  void existenceQCOMPAREs() {
    auto st = QSharedPointer<symbol::Table>::create();
    auto st1 = st->addChild();
    auto st2 = st->addChild();
    // Discard reference returned by st.
    st1->reference("hello");
    // QCOMPARE that traversal policy is respected.
    // Table 2 should not be able to see table 1's symbol with kChildren.
    QVERIFY(!symbol::exists(st2, "hello"));
    // But it can see them when it is allowed to QCOMPARE siblings.
    QVERIFY(symbol::exists(st2, "hello", symbol::TraversalPolicy::kSiblings));
    // Trivially the root can always see any symbol.
    QVERIFY(symbol::exists(st, "hello"));
  }
  //  Dave: QCOMPARE symbol directly against pointer
  void existenceRespectsTraversalPolicy() {
    auto st = QSharedPointer<symbol::Table>::create();
    auto st1 = st->addChild();
    auto st2 = st->addChild();
    st1->reference("hello");
    // QCOMPARE that traversal policy is respected.
    // This calls exists from table directly
    QVERIFY(st1->exists("hello"));
    QVERIFY(!st2->exists("hello"));
  }
  void localityOfDefines() {
    auto st = QSharedPointer<symbol::Table>::create();
    auto st1 = st->addChild();
    auto st2 = st->addChild();
    auto x = st1->reference("hello");
    auto y = st2->reference("hello");
    QCOMPARE(x->state, symbol::DefinitionState::kUndefined);
    st1->define(x->name);
    QCOMPARE(x->state, symbol::DefinitionState::kSingle);
    st1->define(x->name);
    QCOMPARE(x->state, symbol::DefinitionState::kMultiple);
    // Defining a local symbol doesn't affect the state of a symbol in another
    // table.
    QCOMPARE(y->state, symbol::DefinitionState::kUndefined);
  }
  void singleValidGlobal() {
    auto st = QSharedPointer<symbol::Table>::create();
    auto st1 = st->addChild();
    auto st2 = st->addChild();
    auto x = st1->reference("hello");
    auto y = st2->reference("hello");
    st1->markGlobal("hello");
    QCOMPARE(x->binding, symbol::Binding::kGlobal);
    QCOMPARE(y->binding, symbol::Binding::kImported);
    QCOMPARE(x->state, symbol::DefinitionState::kUndefined);
    QCOMPARE(y->state, symbol::DefinitionState::kUndefined);
    // QCOMPARE that defining a global symbol also defines its imports.
    st1->define("hello");
    QCOMPARE(x->state, symbol::DefinitionState::kSingle);
    QCOMPARE(y->state, symbol::DefinitionState::kSingle);
    st2->define("hello");
    QCOMPARE(x->state, symbol::DefinitionState::kSingle);
    QCOMPARE(y->state, symbol::DefinitionState::kExternalMultiple);
  }
  void multipleGlobal() {
    auto st = QSharedPointer<symbol::Table>::create();
    auto st1 = st->addChild();
    auto st2 = st->addChild();
    auto x = st1->reference("hello");
    auto y = st2->reference("hello");
    st1->markGlobal("hello");
    st2->markGlobal("hello");
    QCOMPARE(x->binding, symbol::Binding::kGlobal);
    QCOMPARE(y->binding, symbol::Binding::kGlobal);
    QCOMPARE(x->state, symbol::DefinitionState::kExternalMultiple);
    QCOMPARE(y->state, symbol::DefinitionState::kExternalMultiple);
  }

  void externalSymbolTable() {
    auto st1 = QSharedPointer<symbol::Table>::create();
    auto st2 = QSharedPointer<symbol::Table>::create();

    auto x = st1->define("hello");
    auto x_val = QSharedPointer<symbol::value::Constant>::create();
    x_val->setValue({.byteCount = 2, .bitPattern = 0xfeed, .mask = 0x8FFF});
    x->value = x_val;
    st1->markGlobal("hello");

    auto maybe_y = st2->import(*st1, "hello");
    QVERIFY(maybe_y.has_value());
    auto y = maybe_y.value();
    QCOMPARE(x->binding, symbol::Binding::kGlobal);
    QCOMPARE(y->binding, symbol::Binding::kImported);
    QCOMPARE(x->state, symbol::DefinitionState::kSingle);
    QCOMPARE(y->state, symbol::DefinitionState::kSingle);
    QCOMPARE(x->value->value(), y->value->value());
  }
};
#include "two_table.test.moc"
QTEST_MAIN(Symbol2Table)
