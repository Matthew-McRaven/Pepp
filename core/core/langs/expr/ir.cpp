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
#include "core/langs/expr/ir.hpp"
#include <algorithm>
#include <iterator>
#include <limits>
#include <stdexcept>
#include "core/langs/expr/traversal.hpp"
#include "fmt/format.h"

pepp::tc::expr::NodeId pepp::tc::expr::Tree::add(Node node) {
  // Operands must already be present, or the nodes would no longer be in postorder.
  const auto present = [&](NodeId id) { return id < _nodes.size(); };
  if (const auto *unary = std::get_if<Unary>(&node); unary && !present(unary->operand))
    throw std::logic_error("Unary operand is not in the tree");
  else if (const auto *binary = std::get_if<Binary>(&node); binary && !(present(binary->lhs) && present(binary->rhs)))
    throw std::logic_error("Binary operand is not in the tree");
  else if (const auto *parens = std::get_if<Parens>(&node); parens && !present(parens->inner))
    throw std::logic_error("Parenthesized operand is not in the tree");
  _kinds.emplace_back(kind(node));
  _nodes.emplace_back(std::move(node));
  return static_cast<NodeId>(_nodes.size() - 1);
}

std::string_view pepp::tc::expr::to_string(UnaryOp op) {
  switch (op) {
  case UnaryOp::Plus: return "+";
  case UnaryOp::Minus: return "-";
  case UnaryOp::BitNot: return "~";
  case UnaryOp::LogicalNot: return "!";
  }
  return "?";
}

std::optional<pepp::tc::expr::UnaryOp> pepp::tc::expr::unary_op(std::string_view text) {
  if (text == "+") return UnaryOp::Plus;
  else if (text == "-") return UnaryOp::Minus;
  else if (text == "~") return UnaryOp::BitNot;
  else if (text == "!") return UnaryOp::LogicalNot;
  return std::nullopt;
}

std::string_view pepp::tc::expr::to_string(BinaryOp op) {
  switch (op) {
  case BinaryOp::Multiply: return "*";
  case BinaryOp::Divide: return "/";
  case BinaryOp::Modulo: return "%";
  case BinaryOp::Add: return "+";
  case BinaryOp::Subtract: return "-";
  case BinaryOp::ShiftLeft: return "<<";
  case BinaryOp::ShiftRight: return ">>";
  case BinaryOp::Less: return "<";
  case BinaryOp::LessEqual: return "<=";
  case BinaryOp::Greater: return ">";
  case BinaryOp::GreaterEqual: return ">=";
  case BinaryOp::Equal: return "==";
  case BinaryOp::NotEqual: return "!=";
  case BinaryOp::BitAnd: return "&";
  case BinaryOp::BitXor: return "^";
  case BinaryOp::BitOr: return "|";
  case BinaryOp::LogicalAnd: return "&&";
  case BinaryOp::LogicalOr: return "||";
  }
  return "?";
}
std::optional<pepp::tc::expr::BinaryOp> pepp::tc::expr::binary_op(std::string_view text) {
  using enum BinaryOp;
  if (text == "*") return Multiply;
  else if (text == "/") return Divide;
  else if (text == "%") return Modulo;
  else if (text == "+") return Add;
  else if (text == "-") return Subtract;
  else if (text == "<<") return ShiftLeft;
  else if (text == ">>") return ShiftRight;
  else if (text == "<") return Less;
  else if (text == "<=") return LessEqual;
  else if (text == ">") return Greater;
  else if (text == ">=") return GreaterEqual;
  else if (text == "==") return Equal;
  else if (text == "!=") return NotEqual;
  else if (text == "&") return BitAnd;
  else if (text == "^") return BitXor;
  else if (text == "|") return BitOr;
  else if (text == "&&") return LogicalAnd;
  else if (text == "||") return LogicalOr;
  return std::nullopt;
}

int pepp::tc::expr::precedence(BinaryOp op) {
  switch (op) {
  case BinaryOp::LogicalOr: return 1;
  case BinaryOp::LogicalAnd: return 2;
  case BinaryOp::BitOr: return 3;
  case BinaryOp::BitXor: return 4;
  case BinaryOp::BitAnd: return 5;
  case BinaryOp::Equal: [[fallthrough]];
  case BinaryOp::NotEqual: return 6;
  case BinaryOp::Less: [[fallthrough]];
  case BinaryOp::LessEqual: [[fallthrough]];
  case BinaryOp::Greater: [[fallthrough]];
  case BinaryOp::GreaterEqual: return 7;
  case BinaryOp::ShiftLeft: [[fallthrough]];
  case BinaryOp::ShiftRight: return 8;
  case BinaryOp::Add: [[fallthrough]];
  case BinaryOp::Subtract: return 9;
  case BinaryOp::Multiply: [[fallthrough]];
  case BinaryOp::Divide: [[fallthrough]];
  case BinaryOp::Modulo: return 10;
  }
  return 0;
}

pepp::tc::expr::Kind pepp::tc::expr::kind(const Node &node) {
  auto f = [](const auto &n) -> Kind {
    using T = std::decay_t<decltype(n)>;
    if constexpr (std::is_same_v<T, Unary> || std::is_same_v<T, Binary>) return kind(n.op);
    else return T::KIND;
  };
  return std::visit(f, node);
}

namespace {
using pepp::tc::expr::Character;
using pepp::tc::expr::FoldedConstant;
using pepp::tc::expr::Identifier;
using pepp::tc::expr::Integer;

void append(std::string &out, const Integer &n) {
  if (n.format == Integer::Format::Hexadecimal) fmt::format_to(std::back_inserter(out), "0x{:X}", n.value);
  else fmt::format_to(std::back_inserter(out), "{}", n.value);
}
void append(std::string &out, const Character &n) { fmt::format_to(std::back_inserter(out), "'{}'", n.text); }
void append(std::string &out, const Identifier &n) { out += n.name; }
// Treat folded constants like a conditionally signed integer for serialization purposes.
void append(std::string &out, const FoldedConstant &n) {
  if (n.value.type.sign == pepp::tc::expr::Signedness::Signed)
    fmt::format_to(std::back_inserter(out), "{}", n.value.as_signed());
  else fmt::format_to(std::back_inserter(out), "{}", n.value.bits);
}

void append_infix(std::string &out, const pepp::tc::expr::Tree &tree, pepp::tc::expr::NodeId id);

// Parenthesize operand only when the result would otherwise be ambiguous. Binary operators are left associative, so a
// right operand of equal precedence needs parentheses (a - (b - c)), but a left one does not ((a - b) - c).
void append_operand(std::string &out, const pepp::tc::expr::Tree &tree, pepp::tc::expr::NodeId id, int min_precedence) {
  using namespace pepp::tc::expr;
  const auto *binary = std::get_if<Binary>(&tree[id]);
  const bool parens = binary && precedence(binary->op) < min_precedence;
  if (parens) out += '(';
  append_infix(out, tree, id);
  if (parens) out += ')';
}

void append_infix(std::string &out, const pepp::tc::expr::Tree &tree, pepp::tc::expr::NodeId id) {
  using namespace pepp::tc::expr;
  std::visit(
      [&](const auto &n) {
        using T = std::decay_t<decltype(n)>;
        if constexpr (std::is_same_v<T, Unary>) {
          out += to_string(n.op);
          // Unary operators bind tighter than every binary operator.
          append_operand(out, tree, n.operand, std::numeric_limits<int>::max());
        } else if constexpr (std::is_same_v<T, Binary>) {
          append_operand(out, tree, n.lhs, precedence(n.op));
          fmt::format_to(std::back_inserter(out), " {} ", to_string(n.op));
          append_operand(out, tree, n.rhs, precedence(n.op) + 1);
        } else if constexpr (std::is_same_v<T, Parens>) {
          out += '(';
          append_infix(out, tree, n.inner);
          out += ')';
        } else append(out, n);
      },
      tree[id]);
}
} // namespace

std::string pepp::tc::expr::to_postfix(const Tree &tree) {
  std::string ret;
  auto f = [&](const auto &n) {
    using T = std::decay_t<decltype(n)>;
    if constexpr (std::is_same_v<T, Parens>) return; // Postfix needs no grouping.
    else {
      if (!ret.empty()) ret += ' ';
      if constexpr (std::is_same_v<T, Unary>) {
        if (n.op == UnaryOp::Plus) ret += "u+";
        else if (n.op == UnaryOp::Minus) ret += "u-";
        else ret += to_string(n.op);
      } else if constexpr (std::is_same_v<T, Binary>) ret += to_string(n.op);
      else append(ret, n);
    }
  };
  for (const auto &node : tree.nodes()) std::visit(f, node);
  return ret;
}

std::string pepp::tc::expr::to_infix(const Tree &tree) {
  std::string ret;
  if (!tree.empty()) append_infix(ret, tree, tree.root());
  return ret;
}

pepp::tc::expr::Tree pepp::tc::expr::strip_parens(const Tree &tree) {
  Tree ret;
  const auto rebuild = [&](const auto &self, NodeId id) -> NodeId {
    if (const auto *parens = std::get_if<Parens>(&tree[id])) return self(self, parens->inner);
    return ret.add(map_operands(tree[id], [&](NodeId operand) { return self(self, operand); }));
  };
  if (!tree.empty()) rebuild(rebuild, tree.root());
  return ret;
}

bool pepp::tc::expr::is_constant_expression(const Tree &tree) {
  return !std::ranges::contains(tree.kinds(), Kind::Identifier);
}
