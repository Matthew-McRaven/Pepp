#include "pas/operations/generic/link_globals.hpp"
#include "isa/pep10.hpp"
#include "pas/driver/common.hpp"
#include "pas/driver/pepp.hpp"
#include "symbol/value.hpp"
#include <QObject>
#include <QTest>

using isa::Pep10;
class PasOpsPepp_LinkGlobals : public QObject {
  Q_OBJECT
private slots:
  void linkIntraTree() {
    QString body = "s:.block 10\n.EXPORT s\nLDWA s,i\n.END\n.END";
    auto globals = QSharedPointer<pas::driver::Globals>::create();
    auto ret =
        pas::driver::pepp::createParser<isa::Pep10>(false)(body, nullptr);
    QVERIFY(!ret.hadError);
    pas::ops::generic::linkGlobals(*ret.root, globals, {u"EXPORT"_qs});
    QVERIFY(globals->contains("s"));
    auto sym = globals->get("s");
    QCOMPARE(sym->binding, symbol::Binding::kGlobal);
  }

  void linkInterTree() {
    QString body = "LDWA s,i";
    auto globals = QSharedPointer<pas::driver::Globals>::create();
    auto otherTable = QSharedPointer<symbol::Table>::create(2);
    otherTable->define("s");
    otherTable->markGlobal("s");
    globals->add(*otherTable->get("s"));

    // Verify that global symbol looks correct
    QVERIFY(globals->contains("s"));
    QCOMPARE(globals->get("s")->binding, symbol::Binding::kGlobal);

    auto ret =
        pas::driver::pepp::createParser<isa::Pep10>(false)(body, nullptr);
    QVERIFY(!ret.hadError);
    pas::ops::generic::linkGlobals(*ret.root, globals, {u"EXPORT"_qs});

    // Verify that local symbol is correct
    QVERIFY(ret.root->has<pas::ast::generic::SymbolTable>());
    auto thisTable = ret.root->get<pas::ast::generic::SymbolTable>().value;
    QVERIFY(thisTable->exists("s"));
    auto thisSym = *thisTable->get("s");
    QCOMPARE(thisSym->binding, symbol::Binding::kImported);
    QCOMPARE(thisSym->state, symbol::DefinitionState::kSingle);
    auto casted =
        dynamic_cast<symbol::value::ExternalPointer *>(&*thisSym->value);
    QCOMPARE_NE(casted, nullptr);
    QCOMPARE(casted->symbol_pointer, *otherTable->get("s"));
    QCOMPARE(casted->symbol_table, otherTable);
  }
  void multiDefine() {
    QString body = "s:LDWA s,i";
    auto globals = QSharedPointer<pas::driver::Globals>::create();
    auto otherTable = QSharedPointer<symbol::Table>::create(2);
    otherTable->define("s");
    otherTable->markGlobal("s");
    globals->add(*otherTable->get("s"));

    // Verify that global symbol looks correct
    QVERIFY(globals->contains("s"));
    QCOMPARE(globals->get("s")->binding, symbol::Binding::kGlobal);

    auto ret =
        pas::driver::pepp::createParser<isa::Pep10>(false)(body, nullptr);
    QVERIFY(!ret.hadError);
    pas::ops::generic::linkGlobals(*ret.root, globals, {u"EXPORT"_qs});

    // Verify that local symbol is correct
    QVERIFY(ret.root->has<pas::ast::generic::SymbolTable>());
    auto thisTable = ret.root->get<pas::ast::generic::SymbolTable>().value;
    QVERIFY(thisTable->exists("s"));
    auto thisSym = *thisTable->get("s");
    QCOMPARE(thisSym->binding, symbol::Binding::kImported);
    QCOMPARE(thisSym->state, symbol::DefinitionState::kExternalMultiple);
  }
};

#include "link_globals.moc"

QTEST_MAIN(PasOpsPepp_LinkGlobals)
