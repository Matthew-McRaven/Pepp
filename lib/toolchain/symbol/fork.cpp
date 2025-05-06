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

#include "./fork.hpp"
#include "toolchain/symbol/entry.hpp"
#include "toolchain/symbol/table.hpp"
#include "toolchain/symbol/value.hpp"
#include "value.hpp"
void symbol::ForkMap::addMapping(const Table *from, QSharedPointer<Table> to) { _tableMap[from] = to; }

void symbol::ForkMap::addMapping(const symbol::Entry *from, QSharedPointer<symbol::Entry> to) { _symbolMap[from] = to; }

QSharedPointer<symbol::Table> symbol::ForkMap::map(const Table *from) { return _tableMap.value(from, nullptr); }

QSharedPointer<symbol::Entry> symbol::ForkMap::map(const Entry *from) { return _symbolMap.value(from, nullptr); }

void forkTables(symbol::ForkMap &map, QSharedPointer<const symbol::Table> from,
                QSharedPointer<symbol::Table> toParent) {
  map.addMapping(&*from, toParent);
  for (auto childIt = from->cbegin(); childIt != from->cend(); ++childIt) forkTables(map, (*childIt), toParent);
}

void forkSymbolRefs(symbol::ForkMap &map, QSharedPointer<const symbol::Table> from,
                    QSharedPointer<symbol::Table> toParent) {
  for (const auto &fromSymbolPair : from->entries()) {
    QSharedPointer<symbol::Entry> fromSymbol = fromSymbolPair.second;
    auto toSymbol = toParent->reference(fromSymbol->name);
    toSymbol->binding = fromSymbol->binding;
    toSymbol->state = fromSymbol->state;
    toSymbol->section_index = fromSymbol->section_index;
    map.addMapping(&*(fromSymbol), toSymbol);
  }
  for (auto childIt = from->cbegin(); childIt != from->cend(); ++childIt) forkSymbolRefs(map, (*childIt), toParent);
}

void forkSymbolValues(symbol::ForkMap &map, QSharedPointer<const symbol::Table> from,
                      QSharedPointer<symbol::Table> toParent) {
  for (const auto &fromSymbolPair : from->entries()) {
    auto fromSymbol = fromSymbolPair.second;
    auto toSymbol = map.map(&*fromSymbol);
    if (auto asPointerVal = dynamic_cast<symbol::value::InternalPointer *>(&*fromSymbol->value);
        asPointerVal != nullptr) {
      toSymbol->value = QSharedPointer<symbol::value::InternalPointer>::create(asPointerVal->size(),
                                                                               map.map(&*asPointerVal->symbol_pointer));
    } else {
      toSymbol->value = fromSymbol->value->clone();
    }
  }
}

QSharedPointer<symbol::ForkMap> symbol::fork(QSharedPointer<const Table> from) {
  auto map = QSharedPointer<ForkMap>::create();
  auto to = QSharedPointer<symbol::Table>::create(from->pointerSize());
  forkTables(*map, from, to);
  forkSymbolRefs(*map, from, to);
  forkSymbolValues(*map, from, to);
  return map;
}
