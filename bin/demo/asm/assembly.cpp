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

#include "./assembly.hpp"
#include "builtins/book.hpp"
#include "builtins/registry.hpp"
#include "isa/pep10.hpp"
#include "macro/registry.hpp"
#include "asm/pas/driver/pep10.hpp"
#include "asm/pas/operations/pepp/string.hpp"

void AssemblyManger::onSelectionChanged(builtins::Figure *figure) {
  qDebug() << "Selection changed";
  _active = figure;
}

void AssemblyManger::onAssemble() {
  qDebug() << "Assembly triggered";
  if (_active == nullptr)
    return;

  auto bookRegistry = builtins::Registry(nullptr);
  auto book = bookRegistry.findBook("Computer Systems, 6th Edition");
  auto registry = QSharedPointer<macro::Registry>::create();
  for (auto &macro : book->macros())
    registry->registerMacro(macro::types::Core, macro);

  auto userBody = _active->typesafeElements()["pep"]->contents;
  auto os = _active->defaultOS();
  auto osBody = os->typesafeElements()["pep"]->contents;

  auto pipe = pas::driver::pep10::pipeline<pas::driver::ANTLRParserTag>(
      {{osBody, {.isOS = true}}, {userBody, {.isOS = false}}}, registry);

  Q_ASSERT(pipe->assemble(pas::driver::pep10::Stage::End));

  auto osTarget = pipe->pipelines[0].first;
  Q_ASSERT(osTarget->bodies.contains(pas::driver::repr::Nodes::name));
  auto osRoot = osTarget->bodies[pas::driver::repr::Nodes::name]
                    .value<pas::driver::repr::Nodes>()
                    .value;
  this->_osTxt = pas::ops::pepp::formatListing<isa::Pep10>(*osRoot).join("\n");
  emit osTxtChanged();

  auto userTarget = pipe->pipelines[1].first;
  Q_ASSERT(userTarget->bodies.contains(pas::driver::repr::Nodes::name));
  auto userRoot = userTarget->bodies[pas::driver::repr::Nodes::name]
                      .value<pas::driver::repr::Nodes>()
                      .value;
  this->_usrTxt =
      pas::ops::pepp::formatListing<isa::Pep10>(*userRoot).join("\n");
  emit usrTxtChanged();
}

void AssemblyManger::clearUsrTxt() {
  _usrTxt = "";
  emit usrTxtChanged();
}

void AssemblyManger::clearOsTxt() {
  _osTxt = "";
  emit osTxtChanged();
}
