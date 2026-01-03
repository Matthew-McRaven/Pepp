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

#include "entry.hpp"
#include "table.hpp"
symbol::Entry::Entry(symbol::Table &parent, QString name)
    : parent(parent), state(DefinitionState::kUndefined), name(name), binding(Binding::kLocal),
      value(QSharedPointer<symbol::value::Empty>::create(0)) {}

bool symbol::Entry::is_singly_defined() const { return state == DefinitionState::kSingle; }

bool symbol::Entry::is_undefined() const { return state == DefinitionState::kUndefined; }
