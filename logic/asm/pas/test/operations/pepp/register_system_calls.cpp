#include "pas/operations/pepp/register_system_calls.hpp"
#include "isa/pep10.hpp"
#include "macro/macro.hpp"
#include "macro/registered.hpp"
#include "macro/registry.hpp"
#include "pas/ast/generic/attr_children.hpp"
#include "pas/driver/pep10.hpp"
#include "pas/driver/pepp.hpp"
#include <QObject>
#include <QTest>

typedef void (*testFn)(macro::Registry *);

void unary_test(macro::Registry *registry) {
  QVERIFY(registry->contains("s"));
  auto macro = registry->findMacro("s");
  QCOMPARE(macro->type(), macro::types::System);
  auto contents = macro->contents();
  QCOMPARE(contents->name(), u"s"_qs);
  QCOMPARE(contents->argCount(), 0);
  QCOMPARE(contents->body(), u"LDWA s, i\nUSCALL\n"_qs);
}

void nonunary_test(macro::Registry *registry) {
  QVERIFY(registry->contains("s"));
  auto macro = registry->findMacro("s");
  QCOMPARE(macro->type(), macro::types::System);
  auto contents = macro->contents();
  QCOMPARE(contents->name(), u"s"_qs);
  QCOMPARE(contents->argCount(), 2);
  QCOMPARE(contents->body(), u"LDWA s, i\nSCALL $1, $2\n"_qs);
}

void duplicates_test(macro::Registry *macro) {
  // Nothing to verify, program is in invalid state.
}

using isa::Pep10;
class PasOpsPepp_RegisterSystemCalls : public QObject {
  Q_OBJECT
private slots:
  void smoke() {
    QFETCH(QString, input);
    QFETCH(testFn, validate);
    QFETCH(bool, useDriver);
    QFETCH(bool, errors);

    auto registry = QSharedPointer<macro::Registry>::create();
    QSharedPointer<pas::ast::Node> root;
    if (useDriver) {
      auto pipeline = pas::driver::pep10::stages(input, {.isOS = false});
      auto pipelines = pas::driver::Pipeline<pas::driver::pep10::Stage>{};
      pipelines.pipelines.push_back(pipeline);
      pipelines.globals = QSharedPointer<pas::driver::Globals>::create();
      pipelines.globals->macroRegistry = registry;
      QCOMPARE(pipelines.assemble(pas::driver::pep10::Stage::RegisterExports),
               !errors);
      if (!errors)
        QCOMPARE(pipelines.pipelines[0].first->stage,
                 pas::driver::pep10::Stage::AssignAddresses);
      else
        QCOMPARE(pipelines.pipelines[0].first->stage,
                 pas::driver::pep10::Stage::RegisterExports);
      QVERIFY(pipelines.pipelines[0].first->bodies.contains(
          pas::driver::repr::Nodes::name));
      root = pipelines.pipelines[0]
                 .first->bodies[pas::driver::repr::Nodes::name]
                 .value<pas::driver::repr::Nodes>()
                 .value;
    } else {
      auto parseRoot = pas::driver::pepp::createParser<isa::Pep10>(false);
      auto res = parseRoot(input, nullptr);
      QVERIFY(!res.hadError);
      QCOMPARE(pas::ops::pepp::registerSystemCalls(*res.root, registry),
               !errors);
      root = res.root;
    }
    if (!errors) {
      QVERIFY(!root.isNull());
      validate(&*registry);
    }
  }

  void smoke_data() {
    QTest::addColumn<QSharedPointer<macro::Registry>>("registry");
    QTest::addColumn<QString>("input");
    QTest::addColumn<testFn>("validate");
    QTest::addColumn<bool>("useDriver");
    QTest::addColumn<bool>("errors");

    // Unary
    {
      auto registry = QSharedPointer<macro::Registry>::create();
      QString input = ".USCALL s";
      QTest::addRow("valid unary: visitor")
          << registry << input << &unary_test << false << false;
      QTest::addRow("valid unary: driver")
          << registry << input << &unary_test << true << false;
    }

    // Nonunary
    {
      auto registry = QSharedPointer<macro::Registry>::create();
      QString input = ".SCALL s";
      QTest::addRow("valid nonunary: visitor")
          << registry << input << &nonunary_test << false << false;
      QTest::addRow("valid nonunary: driver")
          << registry << input << &nonunary_test << true << false;
    }

    // Duplicates
    {
      auto registry = QSharedPointer<macro::Registry>::create();
      QString input = ".SCALL s\n.USCALL s";
      QTest::addRow("duplicate systemcalls: visitor")
          << registry << input << &duplicates_test << false << true;
      QTest::addRow("duplicate systemcalls: driver")
          << registry << input << &duplicates_test << true << true;
    }
  }
};

#include "register_system_calls.moc"

QTEST_MAIN(PasOpsPepp_RegisterSystemCalls)
