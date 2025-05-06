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

#include "toolchain/pas/operations/pepp/register_system_calls.hpp"
#include <catch.hpp>
#include "toolchain/pas/ast/generic/attr_children.hpp"
#include "toolchain/pas/driver/pep10.hpp"
#include "toolchain/pas/driver/pepp.hpp"
#include "isa/pep10.hpp"
#include "toolchain/macro/macro.hpp"
#include "toolchain/macro/registered.hpp"
#include "toolchain/macro/registry.hpp"

using testFn = void (*)(macro::Registry *);
using isa::Pep10;
namespace {

void nonunary_test(macro::Registry *registry) {
  REQUIRE(registry->contains("s"));
  auto macro = registry->findMacro("s");
  CHECK(macro->type() == macro::types::System);
  auto contents = macro->contents();
  CHECK(contents->name() == "s");
  CHECK(contents->argCount() == 2);
  CHECK(contents->body() == "LDWA s, i\nSCALL $1, $2\n");
}

void duplicates_test(macro::Registry *macro) {
  // Nothing to verify, program is in invalid state.
}

void smoke(QString input, testFn validate, bool useDriver, bool errors) {
  auto registry = QSharedPointer<macro::Registry>::create();
  QSharedPointer<pas::ast::Node> root;
  if (useDriver) {
    auto pipeline = pas::driver::pep10::stages<pas::driver::ANTLRParserTag>(input, {.isOS = false});
    auto pipelines = pas::driver::Pipeline<pas::driver::pep10::Stage>{};
    pipelines.pipelines.push_back(pipeline);
    pipelines.globals = QSharedPointer<pas::driver::Globals>::create();
    pipelines.globals->macroRegistry = registry;
    CHECK(pipelines.assemble(pas::driver::pep10::Stage::RegisterExports) == !errors);
    if (!errors) CHECK(pipelines.pipelines[0].first->stage == pas::driver::pep10::Stage::AssignAddresses);
    else CHECK(pipelines.pipelines[0].first->stage == pas::driver::pep10::Stage::RegisterExports);
    REQUIRE(pipelines.pipelines[0].first->bodies.contains(pas::driver::repr::Nodes::name));
    root = pipelines.pipelines[0].first->bodies[pas::driver::repr::Nodes::name].value<pas::driver::repr::Nodes>().value;
  } else {
    auto parseRoot = pas::driver::pepp::createParser<isa::Pep10, pas::driver::ANTLRParserTag>(false);
    auto res = parseRoot(input, nullptr);

    REQUIRE_FALSE(res.hadError);
    CHECK(pas::ops::pepp::registerSystemCalls(*res.root, registry) == !errors);
    root = res.root;
  }
  if (!errors) {
    REQUIRE_FALSE(root.isNull());
    validate(&*registry);
  }
}
} // namespace

TEST_CASE("Register system calls", "[scope:asm][kind:unit][arch:pep10]") {
  SECTION("non-unary") {
    QString input = ".SCALL s";
    smoke(input, &nonunary_test, false, false);
    smoke(input, &nonunary_test, true, false);
  }
  SECTION("duplicates") {
    QString input = ".SCALL s\n.SCALL s";
    smoke(input, &duplicates_test, false, true);
    smoke(input, &duplicates_test, true, true);
  }
}
