/*
 * Copyright (c) 2025 J. Stanley Warford, Matthew McRaven
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

#include <catch.hpp>

#include "help/builtins/figure.hpp"
#include "help/builtins/registry.hpp"
#include "sim/debug/expr_parser.hpp"
#include "sim/debug/expr_rtti.hpp"
#include "toolchain/helpers/asmb.hpp"
#include "toolchain/helpers/assemblerregistry.hpp"
#include "toolchain/pas/obj/trace_tags.hpp"

QSharedPointer<const builtins::Book> book(builtins::Registry &reg) {
  QString bookName = "Computer Systems, 6th Edition";
  auto book = reg.findBook(bookName);
  return book;
}

TEST_CASE("Serialize stack ops", "[scope:debug][kind:unit][arch:*]") {
  using namespace pepp::debug;
  using P = types::Primitives;

  auto bookReg = builtins::Registry();
  auto bookPtr = book(bookReg);
  auto os_fig = bookPtr->findFigure("os", "pep10os");
  auto registry = helpers::registry(bookPtr, {});
  REQUIRE(os_fig.get() != nullptr);
  auto text = os_fig->findFragment("pep")->contents();
  REQUIRE(text.size() > 100);
  helpers::AsmHelper asm_helper(registry, text, pepp::Architecture::PEP10);
  asm_helper.setUserText("");
  REQUIRE(asm_helper.assemble());
  auto elf = asm_helper.elf();
  REQUIRE(!elf.isNull());
  auto debugInfo = pas::obj::common::readDebugCommands(*elf);
  CHECK(debugInfo.commands.size() > 0);
  qDebug() << "Debug commands:\n";
  for (auto addr = debugInfo.commands.keyBegin(); addr != debugInfo.commands.keyEnd(); addr++)
    qDebug().noquote() << *addr << debugInfo.commands[*addr];
}
