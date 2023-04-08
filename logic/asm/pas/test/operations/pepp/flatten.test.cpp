#include "pas/operations/generic/flatten.hpp"
#include "macro/macro.hpp"
#include "macro/registry.hpp"
#include "pas/ast/generic/attr_children.hpp"
#include "pas/driver/pep10.hpp"
#include "pas/driver/pepp.hpp"
#include "pas/isa/pep10.hpp"
#include "pas/operations/generic/errors.hpp"
#include "pas/operations/generic/include_macros.hpp"
#include "pas/operations/generic/is.hpp"
#include "pas/operations/pepp/string.hpp"
#include <QObject>
#include <QTest>

using pas::isa::Pep10ISA;
typedef void (*testFn)(QSharedPointer<pas::ast::Node>);

void single_test(QSharedPointer<pas::ast::Node> root) {
  // qWarning() << pas::ops::pepp::formatSource<Pep10ISA>(*root).join("\n");
  QVERIFY(root->has<pas::ast::generic::Children>());
  auto children = root->get<pas::ast::generic::Children>().value;
  QCOMPARE(children.size(), 3);
  for (auto &child : children)
    QVERIFY(!pas::ops::generic::isMacro()(*child));
}

void nesting_test(QSharedPointer<pas::ast::Node> root) {
  // qWarning() << pas::ops::pepp::formatSource<Pep10ISA>(*root).join("\n");
  QVERIFY(root->has<pas::ast::generic::Children>());
  auto children = root->get<pas::ast::generic::Children>().value;
  QCOMPARE(children.size(), 5);
  for (auto &child : children)
    QVERIFY(!pas::ops::generic::isMacro()(*child));
}

class PasOpsPepp_FlattenMacro : public QObject {
  Q_OBJECT
private slots:
  void smoke() {
    QFETCH(QSharedPointer<macro::Registry>, registry);
    QFETCH(testFn, validate);
    QFETCH(bool, useDriver);
    QFETCH(QString, input);

    QSharedPointer<pas::ast::Node> root;
    if (useDriver) {
      auto pipeline = pas::driver::pep10::stages(input, {.isOS = false});
      auto pipelines = pas::driver::Pipeline<pas::driver::pep10::Stage>{};
      pipelines.pipelines.push_back(pipeline);
      pipelines.globals = QSharedPointer<pas::driver::Globals>::create();
      pipelines.globals->macroRegistry = registry;
      QVERIFY(pipelines.assemble(pas::driver::pep10::Stage::FlattenMacros));
      QCOMPARE(pipelines.pipelines[0].first->stage,
               pas::driver::pep10::Stage::GroupNodes);
      QVERIFY(pipelines.pipelines[0].first->bodies.contains(
          pas::driver::repr::Nodes::name));
      root = pipelines.pipelines[0]
                 .first->bodies[pas::driver::repr::Nodes::name]
                 .value<pas::driver::repr::Nodes>()
                 .value;
    } else {
      auto parseRoot = pas::driver::pepp::createParser<Pep10ISA>(false);
      auto res = parseRoot(input, nullptr);
      QVERIFY(!res.hadError);
      auto ret = pas::ops::generic::includeMacros(
          *res.root, pas::driver::pepp::createParser<Pep10ISA>(true), registry);
      root = res.root;
      pas::ops::generic::flattenMacros(*root);
    }
    QVERIFY(!root.isNull());
    validate(root);
  }
  void smoke_data() {
    QTest::addColumn<QSharedPointer<macro::Registry>>("registry");
    QTest::addColumn<QString>("input");
    QTest::addColumn<testFn>("validate");
    QTest::addColumn<bool>("useDriver");

    // Valid non-nesting
    {
      auto registry = QSharedPointer<macro::Registry>::create();
      auto macro = QSharedPointer<macro::Parsed>::create(
          u"alpa"_qs, 0, u".block 1"_qs, u"pep/10"_qs);
      registry->registerMacro(macro::types::Core, macro);
      QString input = "@alpa";
      QTest::addRow("valid non-nesting: visitor")
          << registry << input << &single_test << false;
      QTest::addRow("valid non-nesting: driver")
          << registry << input << &single_test << true;
    }

    // Valid nesting
    {
      auto registry = QSharedPointer<macro::Registry>::create();
      auto macro = QSharedPointer<macro::Parsed>::create(
          u"alpa"_qs, 0, u"@beta"_qs, u"pep/10"_qs);
      registry->registerMacro(macro::types::Core, macro);
      auto macro2 = QSharedPointer<macro::Parsed>::create(
          u"beta"_qs, 0, u".block 1"_qs, u"pep/10"_qs);
      registry->registerMacro(macro::types::Core, macro2);
      QString input = "@alpa";
      QTest::addRow("valid nesting: visitor")
          << registry << input << &nesting_test << false;
      QTest::addRow("valid nesting: driver")
          << registry << input << &nesting_test << true;
    }
  }
};

#include "flatten.test.moc"
QTEST_MAIN(PasOpsPepp_FlattenMacro);
