/*
 * Copyright (c) 2026. Stanley Warford, Matthew McRaven
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once
#include <variant>
#include <vector>
#include "core/langs/expr/ir.hpp"

namespace pepp::tc::expr {

// Return a copy of the current node. If the node has operands, replace the operands with f(operand) before returning
// the node.
template <typename F> Node map_operands(const Node &node, F &&f) {
  if (const auto *unary = std::get_if<Unary>(&node)) return Unary{unary->op, f(unary->operand)};
  else if (const auto *binary = std::get_if<Binary>(&node)) return Binary{binary->op, f(binary->lhs), f(binary->rhs)};
  return node;
}
} // namespace pepp::tc::expr
