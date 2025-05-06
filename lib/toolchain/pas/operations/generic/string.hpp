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

namespace pas::ast {
class Node;
}

struct SourceOptions {
  bool printErrors = false;
  int indentMnemonic = 0;
};
struct ListingOptions {
  SourceOptions source;
  quint16 bytesPerLine = 3;
};

namespace pas::ops::generic::detail {
QString formatErrorsAsComments(const ast::Node &node);
QString format(const QString &symbol, const QString &invoke, const QStringList &args, const QString &comment,
               int indentMnemonic = 0, bool spaceAfterComma = true);
QString formatDirectiveOrMacro(const ast::Node &node, const QString &invoke, SourceOptions opts);
QString formatDirective(const ast::Node &node, SourceOptions opts);
QString formatMacro(const ast::Node &node, SourceOptions opts);
QString formatBlank(const ast::Node &node, SourceOptions opts);
QString formatComment(const ast::Node &node, SourceOptions opts);
} // namespace pas::ops::generic::detail
