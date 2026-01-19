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

#include "toolchain/pas/operations/pepp/assign_addr.hpp"
#include <catch.hpp>
#include "bts/isa/pep/pep10.hpp"
#include "toolchain/macro/declaration.hpp"
#include "toolchain/macro/registry.hpp"
#include "toolchain/pas/ast/generic/attr_address.hpp"
#include "toolchain/pas/driver/pep10.hpp"
#include "toolchain/pas/driver/pepp.hpp"
#include "toolchain/pas/operations/generic/group.hpp"
#include "toolchain/pas/operations/pepp/addressable.hpp"
#include "toolchain/pas/operations/pepp/size.hpp"
#include "toolchain/pas/operations/pepp/string.hpp"
#include "bts/bitmanip/strings.hpp"

using isa::Pep10;
using pas::ops::pepp::Direction;
using namespace Qt::StringLiterals;

namespace {
void childRange(QSharedPointer<pas::ast::Node> parent, qsizetype index, qsizetype start, qsizetype end) {
  REQUIRE(parent->has<pas::ast::generic::Children>());
  auto children = parent->get<pas::ast::generic::Children>().value;
  REQUIRE(children.size() > index);
  auto child = children[index];
  REQUIRE(child->has<pas::ast::generic::Address>());
  auto address = child->get<pas::ast::generic::Address>().value;
  CHECK(address.start == start % 0xFFFF);
  CHECK(address.size == end);
}

typedef void (*testFn)(QSharedPointer<pas::ast::Node>, qsizetype base);

void unary_test(QSharedPointer<pas::ast::Node> root, qsizetype base) {
  auto children = root->get<pas::ast::generic::Children>().value;
  CHECK(children.size() == 2);
  childRange(root, 0, base + 0, 1);
  childRange(root, 1, base + 1, 1);
}

void nonunary_test(QSharedPointer<pas::ast::Node> root, qsizetype base) {
  auto children = root->get<pas::ast::generic::Children>().value;
  CHECK(children.size() == 2);
  childRange(root, 0, base + 0, 3);
  childRange(root, 1, base + 3, 3);
}

void size0_test(QSharedPointer<pas::ast::Node> root, qsizetype base) {
  auto children = root->get<pas::ast::generic::Children>().value;
  CHECK(children.size() == 2);
  // Size 0 directives have no address.
  // childRange(ret.root, 0, base + 0, 0);
  childRange(root, 1, base + 0, 2);
  // childRange(ret.root, 2, base + 2, 0);
}

void ascii2_test(QSharedPointer<pas::ast::Node> root, qsizetype base) {
  auto children = root->get<pas::ast::generic::Children>().value;
  CHECK(children.size() == 2);
  childRange(root, 0, base + 0, 2);
  childRange(root, 1, base + 2, 2);
}

void ascii3_test(QSharedPointer<pas::ast::Node> root, qsizetype base) {
  auto children = root->get<pas::ast::generic::Children>().value;
  CHECK(children.size() == 2);
  childRange(root, 0, base + 0, 2);
  childRange(root, 1, base + 2, 3);
}

void equate_test(QSharedPointer<pas::ast::Node> root, qsizetype base) {
  auto children = root->get<pas::ast::generic::Children>().value;
  CHECK(children.size() == 3);
  childRange(root, 0, 0, 1);
  // EQUATE generates no bytecode, and has no address.
  // childRange(ret.root, 1, 1, 0);
  // childRange(ret.root, 2, 1, 0);
  REQUIRE(children[1]->has<pas::ast::generic::SymbolDeclaration>());
  CHECK(children[1]->get<pas::ast::generic::SymbolDeclaration>().value->value->value()() == 10);
  CHECK(children[2]->get<pas::ast::generic::SymbolDeclaration>().value->value->value()() == 10);
}

void org_test(QSharedPointer<pas::ast::Node> root, qsizetype base) {
  auto children = root->get<pas::ast::generic::Children>().value;
  CHECK(children.size() == 2);
  childRange(root, 1, 0x8000, 3);
}
} // namespace

TEST_CASE("Assign Address", "[scope:asm][kind:unit][arch:pep10]") {
  SECTION("Sequential sections") {
    QString body = u"ldwa 0,i\n.SECTION \"l\"\nldwa 0,i"_s;
    auto pipeline = pas::driver::pep10::stages<pas::driver::ANTLRParserTag>(body, {.isOS = false});
    auto pipelines = pas::driver::Pipeline<pas::driver::pep10::Stage>{};
    pipelines.pipelines.push_back(pipeline);
    pipelines.globals = QSharedPointer<pas::driver::Globals>::create();
    pipelines.globals->macroRegistry = QSharedPointer<macro::Registry>::create();
    REQUIRE(pipelines.assemble(pas::driver::pep10::Stage::AssignAddresses));
    CHECK(pipelines.pipelines[0].first->stage == pas::driver::pep10::Stage::WholeProgramSanity);
    REQUIRE(pipelines.pipelines[0].first->bodies.contains(pas::driver::repr::Nodes::name));
    auto root =
        pipelines.pipelines[0].first->bodies[pas::driver::repr::Nodes::name].value<pas::driver::repr::Nodes>().value;

    // All code is moved from root to section, so we must adjust root
    auto sections = pas::ast::children(*root);
    CHECK(sections.size() == 2);
    CHECK(pas::ast::children(*sections[0]).size() == 1);
    CHECK(pas::ast::children(*sections[1]).size() == 2);
    childRange(sections[0], 0, 0, 3);
    childRange(sections[1], 1, 3, 3);
  }
  SECTION("Sequential ORG sections") {
    QString body = "LDWA 0,i\n"
                   ".SECTION \"s0\"\n"
                   ".ORG 0x8000\n"
                   "LDWA 0,i\n"
                   ".SECTION \"s1\"\n"
                   "LDWA 0,i\n"
                   ".SECTION \"s2\"\n"
                   ".ORG 0x9000\n"
                   "LDWA 0,i\n"
                   ".SECTION \"s3\"\n"
                   "LDWA 0,i";
    auto pipeline = pas::driver::pep10::stages<pas::driver::ANTLRParserTag>(body, {.isOS = false});
    auto pipelines = pas::driver::Pipeline<pas::driver::pep10::Stage>{};
    pipelines.pipelines.push_back(pipeline);
    pipelines.globals = QSharedPointer<pas::driver::Globals>::create();
    pipelines.globals->macroRegistry = QSharedPointer<macro::Registry>::create();
    REQUIRE(pipelines.assemble(pas::driver::pep10::Stage::AssignAddresses));
    CHECK(pipelines.pipelines[0].first->stage == pas::driver::pep10::Stage::WholeProgramSanity);
    REQUIRE(pipelines.pipelines[0].first->bodies.contains(pas::driver::repr::Nodes::name));
    auto root =
        pipelines.pipelines[0].first->bodies[pas::driver::repr::Nodes::name].value<pas::driver::repr::Nodes>().value;
    // All code is moved from root to section, so we must adjust root
    auto sections = pas::ast::children(*root);
    CHECK(sections.size() == 5);
    CHECK(pas::ast::children(*sections[0]).size() == 1);
    CHECK(pas::ast::children(*sections[1]).size() == 3);
    CHECK(pas::ast::children(*sections[2]).size() == 2);
    CHECK(pas::ast::children(*sections[3]).size() == 3);
    CHECK(pas::ast::children(*sections[4]).size() == 2);
    childRange(sections[0], 0, 0x8000 - 3, 3);
    childRange(sections[1], 2, 0x8000, 3);
    childRange(sections[2], 1, 0x9000 - 3, 3);
    childRange(sections[3], 2, 0x9000, 3);
    childRange(sections[4], 1, 0x9000 + 3, 3);
  }

  using align_type = std::tuple<QString, qsizetype, qsizetype, bool>;
  std::list<align_type> aligns{
      // {"ALIGN 1 @ 0"), qsizetype(1), qsizetype(0)},
      {"ALIGN 2 @ 0: visitor", 2, 0, false}, {"ALIGN 4 @ 0: visitor", 4, 0, false},
      {"ALIGN 8 @ 0: visitor", 8, 0, false}, {"ALIGN 2 @ 0: driver", 2, 0, true},
      {"ALIGN 4 @ 0: driver", 4, 0, true},   {"ALIGN 8 @ 0: driver", 8, 0, true},
  };
  for (auto item : aligns) {
    auto [name, align, base, useDriver] = item;
    DYNAMIC_SECTION(name.toStdString()) {
      QString body = u".block 1\n.ALIGN %1\n.block 0"_s.arg(align);
      QSharedPointer<pas::ast::Node> root;
      if (useDriver) {
        auto pipeline = pas::driver::pep10::stages<pas::driver::ANTLRParserTag>(body, {.isOS = false});
        auto pipelines = pas::driver::Pipeline<pas::driver::pep10::Stage>{};
        pipelines.pipelines.push_back(pipeline);
        pipelines.globals = QSharedPointer<pas::driver::Globals>::create();
        pipelines.globals->macroRegistry = QSharedPointer<macro::Registry>::create();
        REQUIRE(pipelines.assemble(pas::driver::pep10::Stage::AssignAddresses));
        CHECK(pipelines.pipelines[0].first->stage == pas::driver::pep10::Stage::WholeProgramSanity);
        REQUIRE(pipelines.pipelines[0].first->bodies.contains(pas::driver::repr::Nodes::name));
        root = pipelines.pipelines[0]
                   .first->bodies[pas::driver::repr::Nodes::name]
                   .value<pas::driver::repr::Nodes>()
                   .value;
      } else {
        auto parseRoot = pas::driver::pepp::createParser<isa::Pep10, pas::driver::ANTLRParserTag>(false);
        auto res = parseRoot(body, nullptr);
        REQUIRE_FALSE(res.hadError);
        pas::ops::generic::groupSections(*res.root, pas::ops::pepp::isAddressable<isa::Pep10>);
        pas::ops::pepp::assignAddresses<isa::Pep10>(*res.root);
        root = res.root;
      }
      // All code is moved from root to section, so we must adjust root
      auto sections = pas::ast::children(*root);
      CHECK(sections.size() == 1);
      root = sections[0];

      auto children = root->get<pas::ast::generic::Children>().value;
      CHECK(children.size() == 3);
      childRange(root, 0, base + 0, 1);
      childRange(root, 1, base + 1, align - 1);
      childRange(root, 2, align, 0);
    }
  }

  using type = std::tuple<QString, qsizetype, bool, QString, testFn>;
  std::list<type> items{
      {"unary @ 0: visitor", 0, false, u"rola\nrolx"_s, &unary_test},
      {"unary @ 0: driver", 0, true, u"rola\nrolx"_s, &unary_test},

      {"nonunary @ 0: visitor", 0, false, u"adda 0,i\nldwa 0,i"_s, &nonunary_test},
      {"nonunary @ 0: driver", 0, true, u"adda 0,i\nldwa 0,i"_s, &nonunary_test},

      {".IMPORT @ 0: visitor", 0, false, u".IMPORT s\n.BLOCK 2"_s, &size0_test},
      {".IMPORT @ 0: driver", 0, true, u".IMPORT s\n.BLOCK 2"_s, &size0_test},
      {".EXPORT @ 0: visitor", 0, false, u".EXPORT s\n.BLOCK 2"_s, &size0_test},
      {".EXPORT @ 0: driver", 0, true, u".EXPORT s\n.BLOCK 2"_s, &size0_test},
      {".SCALL @ 0: visitor", 0, false, u".SCALL s\n.BLOCK 2"_s, &size0_test},
      {".SCALL @ 0: driver", 0, true, u".SCALL s\n.BLOCK 2"_s, &size0_test},
      {".INPUT @ 0: visitor", 0, false, u".INPUT s\n.BLOCK 2"_s, &size0_test},
      {".INPUT @ 0: driver", 0, true, u".INPUT s\n.BLOCK 2"_s, &size0_test},
      {".OUTPUT @ 0: visitor", 0, false, u".OUTPUT s\n.BLOCK 2"_s, &size0_test},
      {".OUTPUT @ 0: driver", 0, true, u".OUTPUT s\n.BLOCK 2"_s, &size0_test},

  };
  QMap<QString, QString> shortArgs = {{"short string, no escaped", "hi"},
                                      {"short string, 1 escaped", ".\\n"},
                                      {"short string, 2 escaped", "\\r\\n"},
                                      {"short string, 2 hex", "\\xff\\x00"}};
  for (auto caseName : shortArgs.keys()) {
    auto input = u".block 2\n.ASCII \"%1\""_s.arg(shortArgs[caseName]);
    auto caseStr = caseName.toStdString();
    items.push_front({u"%1: visitor"_s.arg(caseStr.data()), 0, false, input, &ascii2_test});
    items.push_front({u"%1: driver"_s.arg(caseStr.data()), 0, true, input, &ascii2_test});
  }

  QMap<QString, QString> longArgs = {{"long string, no escaped", "ahi"},
                                     {"long string, 1 escaped", "a.\\n"},
                                     {"long string, 2 escaped", "a\\r\\n"},
                                     {"long string, 2 hex", "a\\xff\\x00"}};
  for (auto caseName : longArgs.keys()) {
    auto input = u".block 2\n.ASCII \"%1\""_s.arg(longArgs[caseName]);
    auto caseStr = caseName.toStdString();
    items.push_front({u"%1: visitor"_s.arg(caseStr.data()), 0, false, input, &ascii3_test});
    items.push_front({u"%1: driver"_s.arg(caseStr.data()), 0, true, input, &ascii3_test});
  }
  items.push_front({".EQUATE @ 0: visitor", 0, false, u".block 1\ns:.EQUATE 10\nn:.EQUATE s"_s, &equate_test});
  items.push_front({".OUTPUT @ 0: driver", 0, true, u".block 1\ns:.EQUATE 10\nn:.EQUATE s"_s, &equate_test});

  items.push_front({".ORG @ 0: visitor", 0, false, u".ORG 0x8000\nldwa 0,i"_s, &org_test});
  items.push_front({".ORG @ 0: driver", 0, true, u".ORG 0x8000\nldwa 0,i"_s, &org_test});
  for (auto item : items) {
    auto [name, base, useDriver, body, validate] = item;
    DYNAMIC_SECTION(name.toStdString()) {
      QSharedPointer<pas::ast::Node> root;
      if (useDriver) {
        auto pipeline = pas::driver::pep10::stages<pas::driver::ANTLRParserTag>(body, {.isOS = false});
        auto pipelines = pas::driver::Pipeline<pas::driver::pep10::Stage>{};
        pipelines.pipelines.push_back(pipeline);
        pipelines.globals = QSharedPointer<pas::driver::Globals>::create();
        pipelines.globals->macroRegistry = QSharedPointer<macro::Registry>::create();
        REQUIRE(pipelines.assemble(pas::driver::pep10::Stage::AssignAddresses));
        CHECK(pipelines.pipelines[0].first->stage == pas::driver::pep10::Stage::WholeProgramSanity);
        REQUIRE(pipelines.pipelines[0].first->bodies.contains(pas::driver::repr::Nodes::name));
        root = pipelines.pipelines[0]
                   .first->bodies[pas::driver::repr::Nodes::name]
                   .value<pas::driver::repr::Nodes>()
                   .value;

      } else {
        auto parseRoot = pas::driver::pepp::createParser<isa::Pep10, pas::driver::ANTLRParserTag>(false);
        auto res = parseRoot(body, nullptr);
        REQUIRE_FALSE(res.hadError);
        pas::ops::generic::groupSections(*res.root, pas::ops::pepp::isAddressable<isa::Pep10>);
        pas::ops::pepp::assignAddresses<isa::Pep10>(*res.root);
        root = res.root;
      }

      // All code is moved from root to section, so we must adjust root
      auto sections = pas::ast::children(*root);
      CHECK(sections.size() == 1);
      root = sections[0];

      validate(root, base);
    }
  }
}
