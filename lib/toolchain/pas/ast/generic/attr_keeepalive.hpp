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
namespace pas::ast::generic {
// Used to extend the lifetime of other AST beyond their lexical scope.
// Really easy to create unreclaimable garbage. Use sparingly.
struct KeepAlive {
  static const inline QString attributeName = "generic:keepalive";
  static const inline uint8_t attribute = 20;
  QList<QSharedPointer<const pas::ast::Node>> keep_alives;
  bool operator==(const KeepAlive &other) const = default;
};
} // namespace pas::ast::generic

Q_DECLARE_METATYPE(pas::ast::generic::KeepAlive);
