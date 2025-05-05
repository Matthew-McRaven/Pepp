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
#include "asm/pas/operations/pepp/size.hpp"
#include <catch.hpp>
#include "asm/pas/driver/pepp.hpp"
#include "asm/pas/operations/generic/include_macros.hpp"
#include "utils/bits/strings.hpp"
#include "isa/pep10.hpp"
#include "macro/macro.hpp"
#include "macro/registry.hpp"

using pas::ops::pepp::Direction;
using pas::ops::pepp::explicitSize;
using namespace Qt::StringLiterals;

TEST_CASE("Size", "[scope:asm][kind:unit][arch:pep10]") {
  SECTION("Unary") {
    QString body = "rola\nrolx";
    auto ret = pas::driver::pepp::createParser<isa::Pep10, pas::driver::ANTLRParserTag>(false)(body, nullptr);
    REQUIRE_FALSE(ret.hadError);
    auto children = ret.root->get<pas::ast::generic::Children>().value;
    CHECK(children.size() == 2);
    for (auto &base : {0, 200, 0xfffe}) {
      CHECK(explicitSize<isa::Pep10>(*ret.root, base, Direction::Forward) == 2);
      CHECK(explicitSize<isa::Pep10>(*ret.root, base, Direction::Backward) == 2);
    }
  }
  SECTION("Nonunary") {
    QString body = "ldwa n,x\nstwa n,x";
    auto ret = pas::driver::pepp::createParser<isa::Pep10, pas::driver::ANTLRParserTag>(false)(body, nullptr);
    REQUIRE_FALSE(ret.hadError);
    auto children = ret.root->get<pas::ast::generic::Children>().value;
    CHECK(children.size() == 2);
    for (auto &base : {0, 200, 0xfffe}) {
      CHECK(explicitSize<isa::Pep10>(*ret.root, base, Direction::Forward) == 6);
      CHECK(explicitSize<isa::Pep10>(*ret.root, base, Direction::Backward) == 6);
    }
  }
  for (auto name : {".IMPORT", ".EXPORT", ".SCALL", ".USCALL", ".INPUT", ".OUTPUT"}) {
    DYNAMIC_SECTION(name) {
      auto ret =
          pas::driver::pepp::createParser<isa::Pep10, pas::driver::ANTLRParserTag>(false)(u"%1 s"_s.arg(name), nullptr);
      REQUIRE_FALSE(ret.hadError);
      auto children = ret.root->get<pas::ast::generic::Children>().value;
      CHECK(children.size() == 1);
      for (auto &base : {0, 200, 0xfffe}) {
        CHECK(explicitSize<isa::Pep10>(*ret.root, base, Direction::Forward) == 0);
        CHECK(explicitSize<isa::Pep10>(*ret.root, base, Direction::Backward) == 0);
      }
    }
  }
  std::vector<std::tuple<std::string, QString>> ascii_cases = {
      {"short string: no escaped", "hi"},    {"short string: 1 escaped", ".\\n"},
      {"short string: 2 escaped", "\\r\\0"}, {"short string: 2 hex", "\\xff\\x00"},
      {"long string: no escaped", "ahi"},    {"long string: 1 escaped", "a.\\n"},
      {"long string: 2 escaped", "a\\r\\0"}, {"long string: 2 hex", "a\\xff\\x00"}};
  for (auto [name, value] : ascii_cases) {
    DYNAMIC_SECTION(name) {
      auto ret = pas::driver::pepp::createParser<isa::Pep10, pas::driver::ANTLRParserTag>(false)(
          u".ASCII \"%1\""_s.arg(value), nullptr);

      REQUIRE_FALSE(ret.hadError);
      auto children = ret.root->get<pas::ast::generic::Children>().value;
      CHECK(children.size() == 1);
      auto len = bits::escapedStringLength(value);
      for (auto &base : {0, 200, 0xfffe}) {
        CHECK(explicitSize<isa::Pep10>(*ret.root, base, Direction::Forward) == len);
        CHECK(explicitSize<isa::Pep10>(*ret.root, base, Direction::Backward) == len);
      }
    }
  }
  std::vector<std::tuple<std::string, qsizetype, qsizetype>> align_cases = {
      {"ALIGN 1 @ 0", 1, 0}, {"ALIGN 1 @ 1", 1, 1}, {"ALIGN 1 @ 2", 1, 2}, {"ALIGN 1 @ FFFE", 1, 0xfffe},
      {"ALIGN 2 @ 0", 2, 0}, {"ALIGN 2 @ 1", 2, 1}, {"ALIGN 2 @ 2", 2, 2}, {"ALIGN 2 @ FFFE", 2, 0xfffe},
      {"ALIGN 4 @ 0", 4, 0}, {"ALIGN 4 @ 1", 4, 1}, {"ALIGN 4 @ 2", 4, 2}, {"ALIGN 4 @ FFFE", 4, 0xfffe},
      {"ALIGN 8 @ 0", 8, 0}, {"ALIGN 8 @ 1", 8, 1}, {"ALIGN 8 @ 2", 8, 2}, {"ALIGN 8 @ FFFE", 8, 0xfffe}};

  for (auto [name, align, base] : align_cases) {
    DYNAMIC_SECTION(name) {
      auto ret = pas::driver::pepp::createParser<isa::Pep10, pas::driver::ANTLRParserTag>(false)(
          u".ALIGN %1"_s.arg(align), nullptr);

      REQUIRE_FALSE(ret.hadError);
      auto children = ret.root->get<pas::ast::generic::Children>().value;
      CHECK(children.size() == 1);
      auto forwardToNextAlign = (align - (base % align)) % align;
      auto forwardEnd = base + forwardToNextAlign;
      auto forwardSize = forwardEnd - base;
      CHECK(explicitSize<isa::Pep10>(*ret.root, base, Direction::Forward) == forwardSize);
      auto backwardStart = base - (base % align);
      auto backwardSize = base - backwardStart;
      CHECK(explicitSize<isa::Pep10>(*ret.root, base, Direction::Backward) == backwardSize);
    }
  }
  constexpr std::array<qsizetype, 4> block_cases = {0x0, 0x1, 0xFFFF, 0x10};
  for (auto count : block_cases) {
    DYNAMIC_SECTION(".BLOCK " << u"%1"_s.arg(count, 0, 16).toStdString()) {
      auto ret = pas::driver::pepp::createParser<isa::Pep10, pas::driver::ANTLRParserTag>(false)(
          u".BLOCK %1"_s.arg(count), nullptr);

      REQUIRE_FALSE(ret.hadError);
      auto children = ret.root->get<pas::ast::generic::Children>().value;
      CHECK(children.size() == 1);
      for (auto &base : {0, 200, 0xfffe}) {
        CHECK(explicitSize<isa::Pep10>(*ret.root, base, Direction::Forward) == count);
        CHECK(explicitSize<isa::Pep10>(*ret.root, base, Direction::Backward) == count);
      }
    }
  }
  SECTION("Word") {
    auto ret = pas::driver::pepp::createParser<isa::Pep10, pas::driver::ANTLRParserTag>(false)(
        u".WORD 0\n .WORD 1\n.WORD 0xFFFF"_s, nullptr);

    REQUIRE_FALSE(ret.hadError);
    auto children = ret.root->get<pas::ast::generic::Children>().value;
    CHECK(children.size() == 3);
    for (auto &base : {0, 200, 0xfffe}) {
      CHECK(explicitSize<isa::Pep10>(*ret.root, base, Direction::Forward) == 6);
      CHECK(explicitSize<isa::Pep10>(*ret.root, base, Direction::Backward) == 6);
    }
  }
  SECTION("Byte") {
    auto ret = pas::driver::pepp::createParser<isa::Pep10, pas::driver::ANTLRParserTag>(false)(
        u".BYTE 0\n .BYTE 1\n.BYTE 0xFF"_s, nullptr);

    REQUIRE_FALSE(ret.hadError);
    auto children = ret.root->get<pas::ast::generic::Children>().value;
    CHECK(children.size() == 3);
    for (auto &base : {0, 200, 0xfffe}) {
      CHECK(explicitSize<isa::Pep10>(*ret.root, base, Direction::Forward) == 3);
      CHECK(explicitSize<isa::Pep10>(*ret.root, base, Direction::Backward) == 3);
    }
  }
  SECTION("Macro") {
    auto registry = QSharedPointer<macro::Registry>::create();
    auto macro = QSharedPointer<macro::Parsed>::create(u"alpha"_s, 0, u".BYTE 1\n.WORD 2"_s, u"pep/10"_s);
    registry->registerMacro(macro::types::Core, macro);
    auto parseRoot = pas::driver::pepp::createParser<isa::Pep10, pas::driver::ANTLRParserTag>(false);
    auto res = parseRoot(u"@alpha\n.END"_s, nullptr);

    REQUIRE_FALSE(res.hadError);
    auto ret = pas::ops::generic::includeMacros(
        *res.root, pas::driver::pepp::createParser<isa::Pep10, pas::driver::ANTLRParserTag>(true), registry);
    REQUIRE(ret);
    CHECK(explicitSize<isa::Pep10>(*res.root, 0, Direction::Forward) == 3);
    CHECK(explicitSize<isa::Pep10>(*res.root, 0, Direction::Backward) == 3);
  }
  SECTION("Empty") {
    auto ret = pas::driver::pepp::createParser<isa::Pep10, pas::driver::ANTLRParserTag>(false)(u"\n\n\n"_s, nullptr);

    REQUIRE_FALSE(ret.hadError);
    auto children = ret.root->get<pas::ast::generic::Children>().value;
    CHECK(children.size() == 3);
    for (auto &base : {0, 200, 0xfffe}) {
      CHECK(explicitSize<isa::Pep10>(*ret.root, base, Direction::Forward) == 0);
      CHECK(explicitSize<isa::Pep10>(*ret.root, base, Direction::Backward) == 0);
    }
  }
}
