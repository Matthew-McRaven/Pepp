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
#include "toolchain/pas/ast/generic/attr_children.hpp"
#include "toolchain/pas/ast/node.hpp"
#include "toolchain/pas/ast/op.hpp"
#include "errors.hpp"
#include "is.hpp"

namespace pas::ops::generic {
struct GroupSections : public pas::ops::MutatingOp<void> {
  GroupSections(QString defaultSectionName, std::function<bool(const ast::Node &)> addressable);
  pas::ast::generic::Children newChildren;
  void operator()(ast::Node &node) override;

private:
  QSharedPointer<pas::ast::Node> currentSection;
  const std::function<bool(const ast::Node &)> addressable;
  bool hasSeenAddressable = false;
};

void groupSections(ast::Node &root, std::function<bool(const ast::Node &)> addressable);
} // namespace pas::ops::generic
