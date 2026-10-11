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
#include <optional>
#include <variant>
#include <vector>
#include "core/langs/expr/ir.hpp"

namespace pepp::tc::expr {

// Return a copy of the current node. If the node has operands, replace the operands with f(operand) before returning
// the node.
template <typename F> Node map_operands(const Node &node, F &&f) {
  if (const auto *unary = std::get_if<Unary>(&node)) return Unary{unary->op, f(unary->operand)};
  else if (const auto *binary = std::get_if<Binary>(&node)) return Binary{binary->op, f(binary->lhs), f(binary->rhs)};
  else if (const auto *parens = std::get_if<Parens>(&node)) return Parens{f(parens->inner)};
  else if (const auto *call = std::get_if<Call>(&node)) return Call{call->function, f(call->argument)};
  return node;
}

// Call f on each node of type T, in postorder.
template <typename T, typename F> void for_each_node(const Tree &tree, F &&f) {
  for (const auto &node : tree.nodes())
    if (const auto *n = std::get_if<T>(&node)) f(*n);
}

// An operator which is associative and commutative under wrapping arithmetic, the operator which undoes it (if any), and
// the value which leaves an operand unchanged. For example, + is undone by - and has an identity of 0.
struct Group {
  BinaryOp op;
  std::optional<BinaryOp> inverse;
  u64 identity;

  // The kinds of the nodes which form a chain of this group.
  constexpr Kind kinds() const {
    using namespace bits;
    return inverse ? kind(op) | kind(*inverse) : kind(op);
  }
};

/*
 * Helpers to perform term-rewriting. Known as linearization in LLVM, this is a helper to turn our binary tree
 * effectively into an n-ary tree combined with the same operation. e.g., converting the tree a + (b + c) to + (a b c)
 * This pass is required for constant folding to be effective.
 */
// An operand of a chain, and whether it is combined using the group's inverse (e.g., subtracted).
struct ChainOperand {
  NodeId id;
  bool inverted;
};

namespace detail {
inline void flatten(const Tree &tree, NodeId id, const Group &group, bool inverted, std::vector<ChainOperand> &out) {
  using namespace bits;
  if (none(tree.kinds()[id] & group.kinds())) return out.push_back({id, inverted});
  const auto &binary = std::get<Binary>(tree[id]);
  flatten(tree, binary.lhs, group, inverted, out);
  flatten(tree, binary.rhs, group, inverted != (binary.op == group.inverse), out);
}
} // namespace detail

// The operands of the chain of group's operators rooted at id, left to right. The right operand of the inverse flips
// inversion, so for + and -, a - (b - c) gives a, inverted b, and c.
inline std::vector<ChainOperand> flatten(const Tree &tree, NodeId id, const Group &group) {
  std::vector<ChainOperand> ret;
  detail::flatten(tree, id, group, false, ret);
  return ret;
}

} // namespace pepp::tc::expr
