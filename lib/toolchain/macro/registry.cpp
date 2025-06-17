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

#include "./registry.hpp"
#include "./macro.hpp"
#include "./registered.hpp"
macro::Registry::Registry(QObject *parent) : QObject{parent} {}

bool macro::Registry::contains(QString name) const { return findMacro(name) != nullptr; }

const macro::Registered *macro::Registry::findMacro(QString name) const {
  auto asUpper = name.toUpper();
  if (auto macro = _macros.constFind(asUpper); macro != _macros.constEnd()) return macro->data();
  else return nullptr;
}

QList<const macro::Registered *> macro::Registry::findMacrosByType(types::Type type) const {
  QList<const Registered *> ret;
  for (const auto &macro : _macros) {
    if (macro->type() == type) ret.push_back(&(*macro));
  }
  return ret;
}

void macro::Registry::clear() {
  this->_macros.clear();
  emit cleared();
}

QSharedPointer<const macro::Registered> macro::Registry::registerMacro(types::Type type, QSharedPointer<Parsed> macro) {
  if (contains(macro->name())) {
    return nullptr;
  }
  auto registered = QSharedPointer<Registered>::create(type, macro);
  _macros[macro->name().toUpper()] = registered;
  emit macrosChanged();
  return registered;
}

void macro::Registry::removeMacro(QString name) {
  auto asUpper = name.toUpper();
  if (!contains(asUpper)) return;
  _macros.remove(asUpper);
  emit macrosChanged();
}
