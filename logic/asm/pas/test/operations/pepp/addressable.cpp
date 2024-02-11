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

#include "asm/pas/operations/pepp/addressable.hpp"
#include <catch.hpp>
#include "asm/pas/ast/node.hpp"
#include "asm/pas/driver/pep10.hpp"
#include "isa/pep10.hpp"

TEST_CASE("Pas Ops, Addressable", "[pas]") {
  auto [name, body, addressable] = GENERATE(table<QString, QString, bool>({
      {"blank", u""_qs, false},
      {"comment", u";hi"_qs, false},

      {"unary", u"ret"_qs, true},
      {"nonunary branch", u"br 20"_qs, true},
      {"nonunary nonbranch", u"ldwa 0,i"_qs, true},

      {".ALIGN", u".ALIGN 2"_qs, true},
      {".ASCII", u".ASCII \"hi\""_qs, true},
      {".BLOCK", u".BLOCK 2"_qs, true},
      {".BURN", u".BURN 0xFFFF"_qs, false},
      {".BYTE", u".BYTE 0xff"_qs, true},
      {".END", u".END"_qs, false},
      {".EQUATE", u"s:.EQUATE 10"_qs, false},
      {".EXPORT", u".EXPORT s"_qs, false},
      {".IMPORT", u".IMPORT s"_qs, false},
      {".INPUT", u".INPUT s"_qs, false},
      {".OUTPUT", u".OUTPUT s"_qs, false},
      {".ORG", u".ORG 0xFAAD"_qs, false},
      {".SCALL", u".SCALL s"_qs, false},
      {".SECTION", u".SECTION \".text\""_qs, false},
      {".USCALL", u".USCALL s"_qs, false},
      {".WORD", u".WORD 10"_qs, true},
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
