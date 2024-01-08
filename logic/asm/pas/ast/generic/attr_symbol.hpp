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

#pragma once
#include <QtCore>
#include "asm/pas/pas_globals.hpp"

namespace symbol {
class Entry;
class Table;
} // namespace symbol

namespace pas::ast::generic {
struct PAS_EXPORT SymbolDeclaration {
  static const inline QString attributeName = u"generic:symbol_decl"_qs;
  QSharedPointer<symbol::Entry> value = {};
  bool operator==(const SymbolDeclaration &other) const = default;
};

struct PAS_EXPORT SymbolTable {
  static const inline QString attributeName = u"generic:symbol_table"_qs;
  QSharedPointer<symbol::Table> value = {};
  bool operator==(const SymbolTable &other) const = default;
};

} // namespace pas::ast::generic
Q_DECLARE_METATYPE(pas::ast::generic::SymbolDeclaration);
Q_DECLARE_METATYPE(pas::ast::generic::SymbolTable);
