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

namespace symbol {
class Entry;
class Table;
} // namespace symbol

namespace pas::ast::generic {
struct SymbolDeclaration {
  static const inline QString attributeName = "generic:symbol_decl";
  static const inline uint8_t attribute = 16;
  QSharedPointer<symbol::Entry> value = {};
  bool operator==(const SymbolDeclaration &other) const = default;
};

struct SymbolTable {
  static const inline QString attributeName = "generic:symbol_table";
  static const inline uint8_t attribute = 17;
  QSharedPointer<symbol::Table> value = {};
  bool operator==(const SymbolTable &other) const = default;
};

} // namespace pas::ast::generic
Q_DECLARE_METATYPE(pas::ast::generic::SymbolDeclaration);
Q_DECLARE_METATYPE(pas::ast::generic::SymbolTable);
