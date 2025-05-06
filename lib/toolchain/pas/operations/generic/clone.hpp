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
#include "toolchain/pas/ast/generic/attr_sec.hpp"
#include "toolchain/pas/ast/op.hpp"

namespace pas::ast {
class Node;
}
namespace symbol {
class Entry;
class Table;
class ForkMap;
} // namespace symbol

namespace pas::ops::generic {
struct clone : public ConstOp<QSharedPointer<ast::Node>> {
  QSharedPointer<symbol::ForkMap> mapping;
  QSharedPointer<ast::Node> operator()(const ast::Node &node) override;

private:
  QSharedPointer<symbol::Entry> entry(const symbol::Entry *entry);
  QSharedPointer<symbol::Table> table(const symbol::Table *table);
};
} // namespace pas::ops::generic
