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

namespace pas::ast {
class Node;
}

struct PAS_EXPORT SourceOptions {
  bool printErrors = false;
};
struct PAS_EXPORT ListingOptions {
  SourceOptions source;
  quint16 bytesPerLine = 3;
};

namespace pas::ops::generic::detail {
QString PAS_EXPORT formatErrorsAsComments(const ast::Node &node);
QString PAS_EXPORT format(QString symbol, QString invoke, QStringList args,
               QString comment);
QString PAS_EXPORT formatDirectiveOrMacro(const ast::Node &node, QString invoke,
                               SourceOptions opts);
QString PAS_EXPORT formatDirective(const ast::Node &node, SourceOptions opts);
QString PAS_EXPORT formatMacro(const ast::Node &node, SourceOptions opts);
QString PAS_EXPORT formatBlank(const ast::Node &node, SourceOptions opts);
QString PAS_EXPORT formatComment(const ast::Node &node, SourceOptions opts);
} // namespace pas::ops::generic::detail
