#include "pas/operations/generic/flatten.hpp"
#include "macro/macro.hpp"
#include "macro/registry.hpp"
#include "pas/ast/generic/attr_children.hpp"
#include "pas/driver/pepp.hpp"
#include "pas/isa/pep10.hpp"
#include "pas/operations/generic/errors.hpp"
#include "pas/operations/generic/is.hpp"
#include "pas/operations/pepp/string.hpp"
#include <QObject>
#include <QTest>

using pas::isa::Pep10ISA;
class PasOpsPepp_FlattenMacro : public QObject {
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
    QCOMPARE(grandchildren.size(), 3);

    // Flatten tree
    pas::ops::generic::flattenMacros(*root);
    QVERIFY(root->has<pas::ast::generic::Children>());
    children = root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 4);
    for (auto &child : children)
      QVERIFY(!pas::ops::generic::isMacro()(*child));
    qWarning() << pas::ops::pepp::formatSource<Pep10ISA>(*root).join("\n");
  }

  void nesting() {
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
    qWarning() << pas::ops::pepp::formatSource<Pep10ISA>(*root).join("\n");
    QVERIFY(root->has<pas::ast::generic::Children>());
    auto children = root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 2);
    QVERIFY(children[0]->has<pas::ast::generic::Children>());
    auto grandchildren = children[0]->get<pas::ast::generic::Children>().value;
    QCOMPARE(grandchildren.size(), 4);
    QVERIFY(grandchildren[1]->has<pas::ast::generic::Children>());
    auto greatgrandchildren =
        grandchildren[1]->get<pas::ast::generic::Children>().value;
    QCOMPARE(greatgrandchildren.size(), 3);

    // Flatten tree
    pas::ops::generic::flattenMacros(*root);
    QVERIFY(root->has<pas::ast::generic::Children>());
    children = root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 7);
    for (auto &child : children)
      QVERIFY(!pas::ops::generic::isMacro()(*child));
  }
};

#include "flatten.test.moc"
QTEST_MAIN(PasOpsPepp_FlattenMacro);
