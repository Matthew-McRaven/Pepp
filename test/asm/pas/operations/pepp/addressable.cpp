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

#include "toolchain/pas/operations/pepp/addressable.hpp"
#include <catch.hpp>
#include "toolchain/pas/ast/node.hpp"
#include "toolchain/pas/driver/pep10.hpp"
#include "isa/pep10.hpp"
using namespace Qt::StringLiterals;

TEST_CASE("Addressable", "[scope:asm][kind:unit][arch:pep10]") {
  auto [name, body, addressable] = GENERATE(table<QString, QString, bool>({
      {"blank", u""_s, false},
      {"comment", u";hi"_s, false},

      {"unary", u"ret"_s, true},
      {"nonunary branch", u"br 20"_s, true},
      {"nonunary nonbranch", u"ldwa 0,i"_s, true},

      {".ALIGN", u".ALIGN 2"_s, true},
      {".ASCII", u".ASCII \"hi\""_s, true},
      {".BLOCK", u".BLOCK 2"_s, true},
      {".BURN", u".BURN 0xFFFF"_s, false},
      {".BYTE", u".BYTE 0xff"_s, true},
      {".END", u".END"_s, false},
      {".EQUATE", u"s:.EQUATE 10"_s, false},
      {".EXPORT", u".EXPORT s"_s, false},
      {".IMPORT", u".IMPORT s"_s, false},
      {".INPUT", u".INPUT s"_s, false},
      {".OUTPUT", u".OUTPUT s"_s, false},
      {".ORG", u".ORG 0xFAAD"_s, false},
      {".SCALL", u".SCALL s"_s, false},
      {".SECTION", u".SECTION \".text\""_s, false},
      {".USCALL", u".USCALL s"_s, false},
      {".WORD", u".WORD 10"_s, true},
  }));
  auto pipeline = pas::driver::pep10::pipeline<pas::driver::ANTLRParserTag>({{body, {.isOS = true}}});
  REQUIRE(pipeline->assemble(pas::driver::pep10::Stage::Parse));
  auto target = pipeline->pipelines[0].first;
  REQUIRE(!target.isNull());
  REQUIRE(target->bodies.contains(pas::driver::repr::Nodes::name));
  auto root = target->bodies[pas::driver::repr::Nodes::name].value<pas::driver::repr::Nodes>().value;
  auto children = pas::ast::children(*root);
  CHECK(children.size() == 1);
  CHECK(pas::ops::pepp::isAddressable<isa::Pep10>(*children[0]) == addressable);
}
