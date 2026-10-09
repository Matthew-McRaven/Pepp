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
#include <iterator>
#include <stdexcept>
#include "fmt/format.h"

pepp::tc::expr::NodeId pepp::tc::expr::Tree::add(Node node, support::LocationInterval location) {
  // Operands must already be present, or the nodes would no longer be in postorder.
  const auto present = [&](NodeId id) { return id < _nodes.size(); };
  if (const auto *unary = std::get_if<Unary>(&node); unary && !present(unary->operand))
    throw std::logic_error("Unary operand is not in the tree");
  else if (const auto *binary = std::get_if<Binary>(&node); binary && !(present(binary->lhs) && present(binary->rhs)))
    throw std::logic_error("Binary operand is not in the tree");
  _kinds.emplace_back(kind(node));
  _nodes.emplace_back(std::move(node));
  _locations.emplace_back(location);
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

std::string pepp::tc::expr::to_postfix(const Tree &tree) {
  std::string ret;
  auto f = [&](const auto &n) {
    using T = std::decay_t<decltype(n)>;
    if constexpr (std::is_same_v<T, Integer>) {
      if (n.format == Integer::Format::Hexadecimal) fmt::format_to(std::back_inserter(ret), "0x{:X}", n.value);
      else fmt::format_to(std::back_inserter(ret), "{}", n.value);
    } else if constexpr (std::is_same_v<T, Character>) fmt::format_to(std::back_inserter(ret), "'{}'", n.text);
    else if constexpr (std::is_same_v<T, Identifier>) ret += n.name;
    else if constexpr (std::is_same_v<T, Unary>) {
      if (n.op == UnaryOp::Plus) ret += "u+";
      else if (n.op == UnaryOp::Minus) ret += "u-";
      else ret += to_string(n.op);
    } else ret += to_string(n.op);
  };
  for (const auto &node : tree.nodes()) {
    if (!ret.empty()) ret += ' ';
    std::visit(f, node);
  }
  return ret;
}
