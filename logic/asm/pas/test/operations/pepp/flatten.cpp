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

#include "asm/pas/operations/generic/flatten.hpp"
#include "isa/pep10.hpp"
#include "macro/macro.hpp"
#include "macro/registry.hpp"
#include "asm/pas/ast/generic/attr_children.hpp"
#include "asm/pas/driver/pep10.hpp"
#include "asm/pas/driver/pepp.hpp"
#include "asm/pas/operations/generic/errors.hpp"
#include "asm/pas/operations/generic/include_macros.hpp"
#include "asm/pas/operations/generic/is.hpp"
#include "asm/pas/operations/pepp/string.hpp"
#include <QObject>
#include <QTest>

using isa::Pep10;
typedef void (*testFn)(QSharedPointer<pas::ast::Node>);

void single_test(QSharedPointer<pas::ast::Node> root) {
  // qWarning() << pas::ops::pepp::formatSource<isa::Pep10>(*root).join("\n");
  QVERIFY(root->has<pas::ast::generic::Children>());
  auto children = root->get<pas::ast::generic::Children>().value;
  QCOMPARE(children.size(), 3);
  for (auto &child : children)
    QVERIFY(!pas::ops::generic::isMacro()(*child));
}

void nesting_test(QSharedPointer<pas::ast::Node> root) {
  // qWarning() << pas::ops::pepp::formatSource<isa::Pep10>(*root).join("\n");
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
      auto pipeline = pas::driver::pep10::stages<pas::driver::ANTLRParserTag>(input, {.isOS = false});
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
      auto parseRoot = pas::driver::pepp::createParser<isa::Pep10, pas::driver::ANTLRParserTag>(false);
      auto res = parseRoot(input, nullptr);
      QVERIFY(!res.hadError);
      auto ret = pas::ops::generic::includeMacros(
          *res.root, pas::driver::pepp::createParser<isa::Pep10, pas::driver::ANTLRParserTag>(true), registry);
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

#include "flatten.moc"
QTEST_MAIN(PasOpsPepp_FlattenMacro);
