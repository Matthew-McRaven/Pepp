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

#include "asm/pas/operations/generic/include_macros.hpp"
#include "isa/pep10.hpp"
#include "macro/macro.hpp"
#include "macro/registry.hpp"
#include "asm/pas/ast/generic/attr_children.hpp"
#include "asm/pas/driver/pep10.hpp"
#include "asm/pas/driver/pepp.hpp"
#include "asm/pas/operations/generic/errors.hpp"
#include <QObject>
#include <QTest>

typedef void (*testFn)(QSharedPointer<pas::ast::Node>);

void success_test(QSharedPointer<pas::ast::Node> root) {
  QVERIFY(root->has<pas::ast::generic::Children>());
  auto children = root->get<pas::ast::generic::Children>().value;
  QCOMPARE(children.size(), 1);
  QVERIFY(children[0]->has<pas::ast::generic::Children>());
  auto grandchildren = children[0]->get<pas::ast::generic::Children>().value;
  QCOMPARE(grandchildren.size(), 3);
}

void errorOnIncorrectArgCount_test(QSharedPointer<pas::ast::Node> root) {
  QVERIFY(root->has<pas::ast::generic::Children>());
  auto children = root->get<pas::ast::generic::Children>().value;
  QCOMPARE(children.size(), 1);
  QVERIFY(children[0]->has<pas::ast::generic::Error>());
  auto child_errors = children[0]->get<pas::ast::generic::Error>().value;
  QCOMPARE(child_errors.size(), 1);
  QCOMPARE(child_errors[0].message,
           pas::errors::pepp::macroWrongArity.arg("alpa").arg(2).arg(0));
}

void errorOnUndefined_test(QSharedPointer<pas::ast::Node> root) {
  QVERIFY(root->has<pas::ast::generic::Children>());
  auto children = root->get<pas::ast::generic::Children>().value;
  QCOMPARE(children.size(), 1);
  QVERIFY(children[0]->has<pas::ast::generic::Error>());
  auto child_errors = children[0]->get<pas::ast::generic::Error>().value;
  QCOMPARE(child_errors.size(), 1);
  QCOMPARE(child_errors[0].message, pas::errors::pepp::noSuchMacro.arg("alpa"));
}

void validNesting_test(QSharedPointer<pas::ast::Node> root) {
  QVERIFY(root->has<pas::ast::generic::Children>());
  auto children = root->get<pas::ast::generic::Children>().value;
  QCOMPARE(children.size(), 1);
  QVERIFY(children[0]->has<pas::ast::generic::Children>());
  auto grandchildren = children[0]->get<pas::ast::generic::Children>().value;
  QCOMPARE(grandchildren.size(), 3);
  QVERIFY(grandchildren[1]->has<pas::ast::generic::Children>());
  auto greatgrandchildren =
      grandchildren[1]->get<pas::ast::generic::Children>().value;
  QCOMPARE(greatgrandchildren.size(), 3);
}

using isa::Pep10;
class PasOpsPepp_IncludeMacro : public QObject {
  Q_OBJECT

private slots:
  void smoke() {

    QFETCH(QSharedPointer<macro::Registry>, registry);
    QFETCH(testFn, validate);
    QFETCH(bool, useDriver);
    QFETCH(bool, errors);
    QFETCH(QString, input);

    QSharedPointer<pas::ast::Node> root;
    if (useDriver) {
      auto pipeline = pas::driver::pep10::stages<pas::driver::ANTLRParserTag>(input, {.isOS = false});
      auto pipelines = pas::driver::Pipeline<pas::driver::pep10::Stage>{};
      pipelines.pipelines.push_back(pipeline);
      pipelines.globals = QSharedPointer<pas::driver::Globals>::create();
      pipelines.globals->macroRegistry = registry;
      QCOMPARE(pipelines.assemble(pas::driver::pep10::Stage::IncludeMacros),
               !errors);
      if (!errors)
        QCOMPARE(pipelines.pipelines[0].first->stage,
                 pas::driver::pep10::Stage::FlattenMacros);
      else
        QCOMPARE(pipelines.pipelines[0].first->stage,
                 pas::driver::pep10::Stage::IncludeMacros);
      QVERIFY(pipelines.pipelines[0].first->bodies.contains(
          pas::driver::repr::Nodes::name));
      root = pipelines.pipelines[0]
                 .first->bodies[pas::driver::repr::Nodes::name]
                 .value<pas::driver::repr::Nodes>()
                 .value;
    } else {
      auto parseRoot = pas::driver::pepp::createParser<isa::Pep10, pas::driver::ANTLRParserTag>(false);
      auto res = parseRoot(input, nullptr);
      QVERIFY(!res.hadError);
      auto ret = pas::ops::generic::includeMacros(
          *res.root, pas::driver::pepp::createParser<isa::Pep10, pas::driver::ANTLRParserTag>(true), registry);
      QCOMPARE(ret, !errors);

      root = res.root;
    }
    QVERIFY(!root.isNull());
    validate(root);
  }

  void smoke_data() {

    QTest::addColumn<QSharedPointer<macro::Registry>>("registry");
    QTest::addColumn<QString>("input");
    QTest::addColumn<testFn>("validate");
    QTest::addColumn<bool>("useDriver");
    QTest::addColumn<bool>("errors");

    // Valid non-nesting
    {
      auto registry = QSharedPointer<macro::Registry>::create();
      auto macro = QSharedPointer<macro::Parsed>::create(
          u"alpa"_qs, 0, u".block 1"_qs, u"pep/10"_qs);
      registry->registerMacro(macro::types::Core, macro);
      QString input = "@alpa";
      QTest::addRow("valid non-nesting: visitor")
          << registry << input << &success_test << false << false;
      QTest::addRow("valid non-nesting: driver")
          << registry << input << &success_test << true << false;
    }

    // Error on incorrect arg count
    {
      auto registry = QSharedPointer<macro::Registry>::create();
      auto macro = QSharedPointer<macro::Parsed>::create(
          u"alpa"_qs, 2, u".END"_qs, u"pep/10"_qs);
      registry->registerMacro(macro::types::Core, macro);
      QString input = "@alpa";
      QTest::addRow("error on incorrect arg count: visitor")
          << registry << input << &errorOnIncorrectArgCount_test << false
          << true;
      QTest::addRow("error on incorrect arg count: driver")
          << registry << input << &errorOnIncorrectArgCount_test << true
          << true;
    }

    // Error on undefined
    {
      auto registry = QSharedPointer<macro::Registry>::create();
      QString input = "@alpa";
      QTest::addRow("error on undefined: visitor")
          << registry << input << &errorOnUndefined_test << false << true;
      QTest::addRow("error on undefined: driver")
          << registry << input << &errorOnUndefined_test << true << true;
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
          << registry << input << &validNesting_test << false << false;
      QTest::addRow("valid nesting: driver")
          << registry << input << &validNesting_test << true << false;
    }
  }

  void rejectsMacroLoops() {}
  void rejectsMacroLoops_data() {
    // 2 items
    // 3 items
    // 4 items
  }
};

#include "include_macros.moc"

QTEST_MAIN(PasOpsPepp_IncludeMacro)
