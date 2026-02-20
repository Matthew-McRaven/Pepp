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
#include "toolchain/pas/ast/op.hpp"

namespace pas::ast {
class Node;
}

namespace pas::ops::generic {
struct addr2line : public ConstOp<void> {
  bool useList = 0;
  std::vector<std::pair<int, quint32>> mapping;
  void operator()(const ast::Node &node) override;
};
std::vector<std::pair<int, quint32>> source2addr(const ast::Node &node);
std::vector<std::pair<int, quint32>> list2addr(const ast::Node &node);
} // namespace pas::ops::generic
