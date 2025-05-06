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

#include "./find.hpp"
#include "toolchain/pas/ast/generic/attr_hide.hpp"
#include "toolchain/pas/operations/generic/is.hpp"
#include "toolchain/pas/operations/pepp/is.hpp"

bool pas::ops::pepp::detail::findNonStructural(const ast::Node &node) {
  return !pas::ops::generic::isStructural()(node);
}

bool pas::ops::pepp::detail::findUnhiddenEnd(const ast::Node &node) {
  return pas::ops::pepp::isEnd()(node) &&
         !(node.has<ast::generic::Hide>() && node.get<ast::generic::Hide>().value.source);
}
