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
#include "toolchain/pas/ast/generic/attr_error.hpp"
#include "toolchain/pas/ast/generic/attr_location.hpp"
#include "toolchain/pas/ast/op.hpp"

namespace pas::ast {
class Node;
} // namespace pas::ast

namespace pas::driver {
struct Globals;
}

namespace symbol {
class Entry;
}

namespace pas::ops::generic {
struct LinkGlobals : public pas::ops::MutatingOp<void> {
  QSharedPointer<pas::driver::Globals> globals;
  QSet<QString> exportDirectives = {};
  void operator()(ast::Node &node);
  void updateSymbol(QSharedPointer<symbol::Entry> symbol);
};

void linkGlobals(ast::Node &node, QSharedPointer<pas::driver::Globals> globals, QSet<QString> exportDirectives);
} // namespace pas::ops::generic
