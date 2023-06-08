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
#include "pas/ast/generic/attr_error.hpp"
#include "pas/ast/generic/attr_location.hpp"
#include "pas/ast/op.hpp"
#include <QtCore>
#include "pas/pas_globals.hpp"

namespace pas::ast {
class Node;
} // namespace pas::ast

namespace pas::ops::generic {
struct PAS_EXPORT CollectErrors : public pas::ops::ConstOp<void> {
  QList<QPair<ast::generic::SourceLocation, ast::generic::Message>> errors;
  void operator()(const ast::Node &node);
};

QList<QPair<ast::generic::SourceLocation, ast::generic::Message>> PAS_EXPORT collectErrors(const ast::Node& node);
} // namespace pas::ops::generic
