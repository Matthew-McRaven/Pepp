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

#include "./errors.hpp"
#include "toolchain/pas/ast/node.hpp"
void pas::ops::generic::CollectErrors::operator()(const ast::Node &node) {
  using Location = ast::generic::SourceLocation;
  using Message = ast::generic::Message;
  using Error = ast::generic::Error;
  if (node.has<Error>()) {
    Location location;
    if (node.has<Location>()) location = node.get<Location>();
    for (auto error : node.get<Error>().value) {
      errors.push_back(QPair<Location, Message>{location, error});
    }
  }
}

QList<QPair<pas::ast::generic::SourceLocation, pas::ast::generic::Message>>
pas::ops::generic::collectErrors(const ast::Node &node) {
  CollectErrors errors;
  pas::ast::apply_recurse(node, errors);
  return errors.errors;
}
