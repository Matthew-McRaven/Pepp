/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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

#include "toolchain/pas/operations/generic/include_macros.hpp"
#include <catch.hpp>
#include "core/arch/pep/isa/pep10.hpp"
#include "toolchain/macro/declaration.hpp"
#include "toolchain/macro/registry.hpp"
#include "toolchain/pas/ast/generic/attr_children.hpp"
#include "toolchain/pas/driver/pep10.hpp"
#include "toolchain/pas/driver/pepp.hpp"
#include "toolchain/pas/errors.hpp"
#include "toolchain/pas/operations/generic/errors.hpp"

using namespace Qt::StringLiterals;

using testFn = void (*)(QSharedPointer<pas::ast::Node>);
namespace {

auto test = pas::driver::pep10::isDirectiveAddressed;
void success_test(QSharedPointer<pas::ast::Node> root) {
  REQUIRE(root->has<pas::ast::generic::Children>());
  auto children = root->get<pas::ast::generic::Children>().value;
  CHECK(children.size() == 1);
  REQUIRE(children[0]->has<pas::ast::generic::Children>());
  auto grandchildren = children[0]->get<pas::ast::generic::Children>().value;
  CHECK(grandchildren.size() == 3);
}

void errorOnIncorrectArgCount_test(QSharedPointer<pas::ast::Node> root) {
  REQUIRE(root->has<pas::ast::generic::Children>());
  auto children = root->get<pas::ast::generic::Children>().value;
  CHECK(children.size() == 1);
  REQUIRE(children[0]->has<pas::ast::generic::Error>());
  auto child_errors = children[0]->get<pas::ast::generic::Error>().value;
  CHECK(child_errors.size() == 1);
  CHECK(child_errors[0].message.toStdString() ==
        pas::errors::pepp::macroWrongArity.arg("alpa").arg(2).arg(0).toStdString());
}

void errorOnUndefined_test(QSharedPointer<pas::ast::Node> root) {
  REQUIRE(root->has<pas::ast::generic::Children>());
  auto children = root->get<pas::ast::generic::Children>().value;
  CHECK(children.size() == 1);
  REQUIRE(children[0]->has<pas::ast::generic::Error>());
  auto child_errors = children[0]->get<pas::ast::generic::Error>().value;
  CHECK(child_errors.size() == 1);
  CHECK(child_errors[0].message.toStdString() == pas::errors::pepp::noSuchMacro.arg("alpa").toStdString());
}

void validNesting_test(QSharedPointer<pas::ast::Node> root) {
  REQUIRE(root->has<pas::ast::generic::Children>());
  auto children = root->get<pas::ast::generic::Children>().value;
  CHECK(children.size() == 1);
  REQUIRE(children[0]->has<pas::ast::generic::Children>());
  auto grandchildren = children[0]->get<pas::ast::generic::Children>().value;
  CHECK(grandchildren.size() == 3);
  REQUIRE(grandchildren[1]->has<pas::ast::generic::Children>());
  auto greatgrandchildren = grandchildren[1]->get<pas::ast::generic::Children>().value;
  CHECK(greatgrandchildren.size() == 3);
}

void smoke(QSharedPointer<macro::Registry> registry, QString input, testFn validate, bool useDriver, bool errors) {

  QSharedPointer<pas::ast::Node> root;
  if (useDriver) {
    auto pipeline = pas::driver::pep10::stages<pas::driver::ANTLRParserTag>(input, {.isOS = false});
    auto pipelines = pas::driver::Pipeline<pas::driver::pep10::Stage>{};
    pipelines.pipelines.push_back(pipeline);
    pipelines.globals = QSharedPointer<pas::driver::Globals>::create();
    pipelines.globals->macroRegistry = registry;
    pipelines.assemble(pas::driver::pep10::Stage::IncludeMacros);
    if (!errors) CHECK(pipelines.pipelines[0].first->stage == pas::driver::pep10::Stage::GroupNodes);
    else CHECK(pipelines.pipelines[0].first->stage == pas::driver::pep10::Stage::IncludeMacros);
    REQUIRE(pipelines.pipelines[0].first->bodies.contains(pas::driver::repr::Nodes::name));
    root = pipelines.pipelines[0].first->bodies[pas::driver::repr::Nodes::name].value<pas::driver::repr::Nodes>().value;
  } else {
    auto parseRoot = pas::driver::pepp::createParser<isa::Pep10, pas::driver::ANTLRParserTag>(false);
    auto res = parseRoot(input, nullptr);
    REQUIRE(!res.hadError);
    auto ret = pas::ops::generic::includeMacros(
        *res.root, pas::driver::pepp::createParser<isa::Pep10, pas::driver::ANTLRParserTag>(true), registry, test);
    REQUIRE(ret == !errors);

    root = res.root;
  }
  REQUIRE(!root.isNull());
  validate(root);
}
} // namespace

TEST_CASE("Include macros", "[scope:asm][kind:unit][arch:pep10]") {
  // Valid non-nesting
  SECTION("Valid non-nesting") {
    auto registry = QSharedPointer<macro::Registry>::create();
    auto macro = QSharedPointer<macro::Declaration>::create(u"alpa"_s, 0, u".block 1"_s, u"pep/10"_s);
    registry->registerMacro(macro::types::Core, macro);
    QString input = "@alpa";
    smoke(registry, input, &success_test, false, false);
    smoke(registry, input, &success_test, true, false);
  }

  // Error on incorrect arg count
  SECTION("Error on incorrect arg count") {
    auto registry = QSharedPointer<macro::Registry>::create();

    auto macro = QSharedPointer<macro::Declaration>::create(u"alpa"_s, 2, u".END"_s, u"pep/10"_s);
    registry->registerMacro(macro::types::Core, macro);
    QString input = "@alpa";
    smoke(registry, input, &errorOnIncorrectArgCount_test, false, true);
    smoke(registry, input, &errorOnIncorrectArgCount_test, true, true);
  }

  // Error on undefined
  SECTION("Error on undefined") {
    auto registry = QSharedPointer<macro::Registry>::create();
    QString input = "@alpa";
    smoke(registry, input, &errorOnUndefined_test, false, true);
    smoke(registry, input, &errorOnUndefined_test, true, true);
  }

  // Valid nesting
  SECTION("Valid nesting") {
    auto registry = QSharedPointer<macro::Registry>::create();
    auto macro = QSharedPointer<macro::Declaration>::create(u"alpa"_s, 0, u"@beta"_s, u"pep/10"_s);
    registry->registerMacro(macro::types::Core, macro);
    auto macro1 = QSharedPointer<macro::Declaration>::create(u"beta"_s, 0, u".block 1"_s, u"pep/10"_s);
    registry->registerMacro(macro::types::Core, macro1);
    QString input = "@alpa";
    smoke(registry, input, &validNesting_test, false, false);
    smoke(registry, input, &validNesting_test, true, false);
  }
  // TODO: Reject macro loops
}
