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

#include "flatten.hpp"
#include "toolchain/pas/ast/generic/attr_children.hpp"
#include "toolchain/pas/ast/node.hpp"
#include "is.hpp"

void pas::ops::generic::flattenMacros(ast::Node &node) {
  if (node.has<ast::generic::Children>()) {
    auto newChildren = QList<QSharedPointer<ast::Node>>{};
    for (auto &child : node.take<ast::generic::Children>().value) {
      flattenMacros(*child);
      if (isMacro()(*child)) {
        for (auto &macroChild : child->get<ast::generic::Children>().value) newChildren.append(macroChild);
      } else newChildren.append(child);
    }

    node.set(ast::generic::Children{.value = newChildren});
  }
  return;
}
