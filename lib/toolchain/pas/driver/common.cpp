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

#include "./common.hpp"
#include "toolchain/symbol/entry.hpp"
#include "toolchain/symbol/table.hpp"

bool pas::driver::Globals::contains(QString symbol) const { return table.contains(symbol); }

QSharedPointer<symbol::Entry> pas::driver::Globals::get(QString symbol) { return table[symbol]; }

bool pas::driver::Globals::add(QSharedPointer<symbol::Entry> symbol) {
  if (contains(symbol->name)) {
    symbol->state = symbol::DefinitionState::kExternalMultiple;
    get(symbol->name)->state = symbol::DefinitionState::kExternalMultiple;
    return false;
  } else table[symbol->name] = symbol;
  return true;
}
