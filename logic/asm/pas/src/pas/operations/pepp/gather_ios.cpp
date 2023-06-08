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

#include "./gather_ios.hpp"
#include "pas/ast/generic/attr_directive.hpp"
#include "pas/ast/value/base.hpp"
#include "pas/operations/generic/is.hpp"
#include "pas/operations/pepp/is.hpp"

void pas::ops::pepp::GatherIODefinitions::operator()(const ast::Node &node) {
  if (!node.has<ast::generic::Directive>())
    return;
  auto directive = node.get<ast::generic::Directive>().value;
  if (!(directive == "INPUT" || directive == "OUTPUT"))
    return;
  ::obj::IO::Direction direction = (directive == "INPUT")
                                       ? ::obj::IO::Direction::kInput
                                       : ::obj::IO::Direction::kOutput;
  auto arg = node.get<ast::generic::Argument>().value;
  auto symbol = arg->rawString();
  ios.append({.name = symbol, .direction = direction});
}

QList<::obj::IO> pas::ops::pepp::gatherIODefinitions(const ast::Node &node) {
  GatherIODefinitions ret;
  generic::Or<pepp::isInput, pepp::isOutput> pred;
  ast::apply_recurse_if(node, pred, ret);
  return ret.ios;
}
