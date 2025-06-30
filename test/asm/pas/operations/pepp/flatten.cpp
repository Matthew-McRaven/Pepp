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

#include "toolchain/pas/operations/generic/flatten.hpp"
#include <catch.hpp>
#include "enums/isa/pep10.hpp"
#include "toolchain/macro/declaration.hpp"
#include "toolchain/macro/registry.hpp"
#include "toolchain/pas/ast/generic/attr_children.hpp"
#include "toolchain/pas/driver/pep10.hpp"
#include "toolchain/pas/driver/pepp.hpp"
#include "toolchain/pas/operations/generic/errors.hpp"
#include "toolchain/pas/operations/generic/include_macros.hpp"
#include "toolchain/pas/operations/generic/is.hpp"
#include "toolchain/pas/operations/pepp/string.hpp"

using isa::Pep10;
using namespace Qt::StringLiterals;

using testFn = void (*)(QSharedPointer<pas::ast::Node>);
namespace {

auto test = pas::driver::pep10::isDirectiveAddressed;
void single_test(QSharedPointer<pas::ast::Node> root) {
  // qWarning() << pas::ops::pepp::formatSource<isa::Pep10>(*root).join("\n");
  REQUIRE(root->has<pas::ast::generic::Children>());
  auto children = root->get<pas::ast::generic::Children>().value;
  CHECK(children.size() == 3);
  for (auto &child : children) REQUIRE_FALSE(pas::ops::generic::isMacro()(*child));
}

void nesting_test(QSharedPointer<pas::ast::Node> root) {
  // qWarning() << pas::ops::pepp::formatSource<isa::Pep10>(*root).join("\n");
  REQUIRE(root->has<pas::ast::generic::Children>());
  auto children = root->get<pas::ast::generic::Children>().value;
  CHECK(children.size() == 5);
  for (auto &child : children) REQUIRE_FALSE(pas::ops::generic::isMacro()(*child));
}
} // namespace

TEST_CASE("Flatten macros", "[scope:asm][kind:unit][arch:pep10]") {
  using type = std::tuple<QString, QSharedPointer<macro::Registry>, QString, testFn, bool>;
  std::list<type> items;
  // Valid non-nesting
  {
    auto registry = QSharedPointer<macro::Registry>::create();
    auto macro = QSharedPointer<macro::Declaration>::create(u"alpa"_s, 0, u".block 1"_s, u"pep/10"_s);
    registry->registerMacro(macro::types::Core, macro);
    QString input = "@alpa";
    items.push_front({"valid non-nesting: visitor", registry, input, &single_test, false});
    items.push_front({"valid non-nesting: driver", registry, input, &single_test, true});
  }

  // Valid nesting
  {
    auto registry = QSharedPointer<macro::Registry>::create();
    auto macro = QSharedPointer<macro::Declaration>::create(u"alpa"_s, 0, u"@beta"_s, u"pep/10"_s);
    registry->registerMacro(macro::types::Core, macro);
    auto macro2 = QSharedPointer<macro::Declaration>::create(u"beta"_s, 0, u".block 1"_s, u"pep/10"_s);
    registry->registerMacro(macro::types::Core, macro2);
    QString input = "@alpa";
    items.push_front({"valid nesting: visitor", registry, input, &nesting_test, false});
    items.push_front({"valid nesting: driver", registry, input, &nesting_test, true});
  }
  for (auto item : items) {
    auto [name, registery, input, validate, useDriver] = item;
    DYNAMIC_SECTION(name.toStdString()) {
      QSharedPointer<pas::ast::Node> root;
      if (useDriver) {
        auto pipeline = pas::driver::pep10::stages<pas::driver::ANTLRParserTag>(input, {.isOS = false});
        auto pipelines = pas::driver::Pipeline<pas::driver::pep10::Stage>{};
        pipelines.pipelines.push_back(pipeline);
        pipelines.globals = QSharedPointer<pas::driver::Globals>::create();
        pipelines.globals->macroRegistry = registery;
        REQUIRE(pipelines.assemble(pas::driver::pep10::Stage::FlattenMacros));
        CHECK(pipelines.pipelines[0].first->stage == pas::driver::pep10::Stage::GroupNodes);
        REQUIRE(pipelines.pipelines[0].first->bodies.contains(pas::driver::repr::Nodes::name));
        root = pipelines.pipelines[0]
                   .first->bodies[pas::driver::repr::Nodes::name]
                   .value<pas::driver::repr::Nodes>()
                   .value;
      } else {
        auto parseRoot = pas::driver::pepp::createParser<isa::Pep10, pas::driver::ANTLRParserTag>(false);
        auto res = parseRoot(input, nullptr);
        REQUIRE(!res.hadError);
        auto ret = pas::ops::generic::includeMacros(
            *res.root, pas::driver::pepp::createParser<isa::Pep10, pas::driver::ANTLRParserTag>(true), registery, test);
        root = res.root;
        pas::ops::generic::flattenMacros(*root);
      }
      REQUIRE(!root.isNull());
      validate(root);
    }
  }
}
