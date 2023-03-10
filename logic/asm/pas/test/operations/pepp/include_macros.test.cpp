#include "macro/macro.hpp"
#include "macro/registry.hpp"
#include "pas/ast/generic/attr_children.hpp"
#include "pas/driver/pepp.hpp"
#include "pas/isa/pep10.hpp"
#include "pas/operations/generic/errors.hpp"
#include <QObject>
#include <QTest>

using pas::isa::Pep10ISA;
class PasOpsPepp_IncludeMacro : public QObject {
  Q_OBJECT
private slots:
  void smokeTest() {
    auto registry = QSharedPointer<macro::Registry>::create();
    auto macro = QSharedPointer<macro::Parsed>::create(
        u"alpa"_qs, 0, u".END"_qs, u"pep/10"_qs);
    registry->registerMacro(macro::types::Core, macro);
    auto parseRoot = pas::driver::pepp::createParser<Pep10ISA>(false);
    auto res = parseRoot(u"@alpa\n.END"_qs, nullptr);
    QVERIFY(!res.hadError);

    auto ret = pas::ops::generic::includeMacros(
        *res.root, pas::driver::pepp::createParser<Pep10ISA>(true), registry);
    QVERIFY(ret);

    // Validate tree
    auto root = res.root;
    QVERIFY(root->has<pas::ast::generic::Children>());
    auto children = root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 2);
    QVERIFY(children[0]->has<pas::ast::generic::Children>());
    auto grandchildren = children[0]->get<pas::ast::generic::Children>().value;
    QCOMPARE(grandchildren.size(), 1);
  }
  void errorOnIncorrectArgCount() {
    auto registry = QSharedPointer<macro::Registry>::create();
    auto macro = QSharedPointer<macro::Parsed>::create(
        u"alpa"_qs, 2, u".END"_qs, u"pep/10"_qs);
    registry->registerMacro(macro::types::Core, macro);
    auto parseRoot = pas::driver::pepp::createParser<Pep10ISA>(false);
    auto res = parseRoot(u"@alpa\n.END"_qs, nullptr);
    QVERIFY(!res.hadError);

    auto ret = pas::ops::generic::includeMacros(
        *res.root, pas::driver::pepp::createParser<Pep10ISA>(true), registry);
    QVERIFY(!ret);

    // Validate tree
    auto root = res.root;
    QVERIFY(root->has<pas::ast::generic::Children>());
    auto children = root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 2);
    QVERIFY(children[0]->has<pas::ast::generic::Error>());
    auto child_errors = children[0]->get<pas::ast::generic::Error>().value;
    QCOMPARE(child_errors.size(), 1);
    QCOMPARE(child_errors[0].message,
             pas::errors::pepp::macroWrongArity.arg("alpa").arg(2).arg(0));
  }
  void errorOnUndefined() {
    auto registry = QSharedPointer<macro::Registry>::create();
    auto errors = pas::ops::generic::CollectErrors();
    auto parseRoot = pas::driver::pepp::createParser<Pep10ISA>(false);
    auto res = parseRoot(u"@alpa\n.END"_qs, nullptr);
    QVERIFY(!res.hadError);

    auto ret = pas::ops::generic::includeMacros(
        *res.root, pas::driver::pepp::createParser<Pep10ISA>(true), registry);
    QVERIFY(!ret);

    // Validate tree
    auto root = res.root;
    QVERIFY(root->has<pas::ast::generic::Children>());
    auto children = root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 2);
    QVERIFY(children[0]->has<pas::ast::generic::Error>());
    auto child_errors = children[0]->get<pas::ast::generic::Error>().value;
    QCOMPARE(child_errors.size(), 1);
    QCOMPARE(child_errors[0].message,
             pas::errors::pepp::noSuchMacro.arg("alpa"));
  }
  void validNesting() {
    auto registry = QSharedPointer<macro::Registry>::create();
    auto macro = QSharedPointer<macro::Parsed>::create(
        u"alpa"_qs, 0, u"@beta\n.END"_qs, u"pep/10"_qs);
    registry->registerMacro(macro::types::Core, macro);
    auto macro2 = QSharedPointer<macro::Parsed>::create(
        u"beta"_qs, 0, u".END"_qs, u"pep/10"_qs);
    registry->registerMacro(macro::types::Core, macro2);
    auto parseRoot = pas::driver::pepp::createParser<Pep10ISA>(false);
    auto res = parseRoot(u"@alpa\n.END"_qs, nullptr);
    QVERIFY(!res.hadError);

    auto ret = pas::ops::generic::includeMacros(
        *res.root, pas::driver::pepp::createParser<Pep10ISA>(true), registry);
    QVERIFY(ret);

    // Validate tree
    auto root = res.root;
    QVERIFY(root->has<pas::ast::generic::Children>());
    auto children = root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 2);
    QVERIFY(children[0]->has<pas::ast::generic::Children>());
    auto grandchildren = children[0]->get<pas::ast::generic::Children>().value;
    QCOMPARE(grandchildren.size(), 2);
    QVERIFY(grandchildren[0]->has<pas::ast::generic::Children>());
    auto greatgrandchildren =
        grandchildren[0]->get<pas::ast::generic::Children>().value;
    QCOMPARE(greatgrandchildren.size(), 1);
  }
  void rejectsMacroLoops() {}
  void rejectsMacroLoops_data() {
    // 2 items
    // 3 items
    // 4 items
  }
};

#include "include_macros.test.moc"

QTEST_MAIN(PasOpsPepp_IncludeMacro)
