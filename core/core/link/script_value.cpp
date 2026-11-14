/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
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

#include "core/link/script_value.hpp"
#include <span>
#include <vector>
#include "core/langs/expr/evaluator.hpp"
#include "core/math/bitmanip/log2.hpp"

namespace {
namespace expr = pepp::tc::expr;
using pepp::link::ScriptContext;
using pepp::link::ScriptValue;

constexpr expr::Type script_type{64, expr::Signedness::Unsigned};

// Computes the value of one node from the values of its operands, which precede it in the tree.
struct ExpresionEvalutionVisitor {
  using enum ScriptValue::Kind;
  const ScriptContext &context;
  // The values of the nodes before id, indexed by NodeId.
  std::span<const std::expected<ScriptValue, expr::EvaluationError>> values;
  expr::NodeId id;

  std::expected<ScriptValue, expr::EvaluationError> operator()(const expr::Integer &integer) const {
    return ScriptValue{Number, integer.value};
  }
  std::expected<ScriptValue, expr::EvaluationError> operator()(const expr::Character &character) const {
    return ScriptValue{Number, character.value};
  }
  std::expected<ScriptValue, expr::EvaluationError> operator()(const expr::FoldedConstant &folded) const {
    return ScriptValue{Number, folded.value.bits};
  }
  std::expected<ScriptValue, expr::EvaluationError> operator()(const expr::LocationCounter &) const {
    return location_counter(context);
  }
  std::expected<ScriptValue, expr::EvaluationError> operator()(const expr::Parens &parens) const {
    return values[parens.inner];
  }
  std::expected<ScriptValue, expr::EvaluationError> operator()(const expr::Identifier &identifier) const {
    return std::unexpected(expr::EvaluationError(id, expr::UnaryError::Symbol_NoValue, identifier.name));
  }
  std::expected<ScriptValue, expr::EvaluationError> operator()(const expr::Unary &unary) const {
    auto operand = values[unary.operand];
    if (!operand) return operand;
    const auto result = expr::apply(unary.op, {operand->value, script_type}, script_type);
    if (!result) return std::unexpected(expr::EvaluationError(id, result.error()));
    operand->value = result->bits;
    return operand;
  }
  std::expected<ScriptValue, expr::EvaluationError> operator()(const expr::Binary &binary) const {
    const auto &lhs = values[binary.lhs], &rhs = values[binary.rhs];
    if (!lhs) return lhs;
    else if (!rhs) return rhs;
    const auto value = combine(binary.op, *lhs, *rhs, context);
    if (!value) return std::unexpected(expr::EvaluationError(id, value.error()));
    return *value;
  }
  // ALIGN is the only function. ALIGN(align) aligns the location counter's address, and ALIGN(exp, align) aligns
  // exp's value without changing its kind. Alignments must be powers of two.
  std::expected<ScriptValue, expr::EvaluationError> operator()(const expr::Call &call) const {
    const auto &alignment = values[call.arguments.back()];
    if (!alignment) return alignment;
    else if (call.arguments.size() == 1) {
      const u64 address = bits::align_up(absolute(location_counter(context), context), alignment->value);
      if (!context.current) return ScriptValue{Absolute, address};
      return ScriptValue{Relative, address - context.address_of(context.current), context.current};
    }
    auto aligned = values[call.arguments.front()];
    if (aligned) aligned->value = bits::align_up(aligned->value, alignment->value);
    return aligned;
  }
};
} // namespace

pepp::link::ScriptValue pepp::link::location_counter(const ScriptContext &context) {
  if (!context.current) return {ScriptValue::Kind::Absolute, context.dot};
  return {ScriptValue::Kind::Relative, context.dot - context.address_of(context.current), context.current};
}

u64 pepp::link::absolute(const ScriptValue &value, const ScriptContext &context) {
  if (value.kind != ScriptValue::Kind::Relative) return value.value;
  return context.address_of(value.section) + value.value;
}

std::expected<pepp::link::ScriptValue, pepp::tc::expr::NullaryError>
pepp::link::combine(tc::expr::BinaryOp op, ScriptValue lhs, ScriptValue rhs, const ScriptContext &context) {
  using enum ScriptValue::Kind;
  const auto apply = [&](u64 l, u64 r) -> std::expected<u64, expr::NullaryError> {
    const auto result = expr::apply(op, {l, script_type}, {r, script_type}, script_type);
    if (!result) return std::unexpected(result.error());
    return result->bits;
  };
  // Comparisons and logical operators produce numbers.
  const bool number_result = op == expr::BinaryOp::LogicalAnd || op == expr::BinaryOp::LogicalOr ||
                             (op >= expr::BinaryOp::Less && op <= expr::BinaryOp::NotEqual);
  // Operations on the offsets within a section, or offsets combined with numbers remain offsets
  if (lhs.kind == Relative && (rhs.kind == Number || (rhs.kind == Relative && rhs.section == lhs.section))) {
    if (const auto value = apply(lhs.value, rhs.value); !value) return std::unexpected(value.error());
    else if (number_result || rhs.kind == Relative) return ScriptValue{context.current ? Number : Absolute, *value};
    else return ScriptValue{Relative, *value, lhs.section};
  } else if (lhs.kind == Number && rhs.kind == Relative) {
    if (const auto value = apply(lhs.value, rhs.value); !value) return std::unexpected(value.error());
    else if (number_result) return ScriptValue{Number, *value};
    else return ScriptValue{Relative, *value, rhs.section};
  }
  // Otherwise, operate on absolute addresses.

  if (const auto value = apply(absolute(lhs, context), absolute(rhs, context)); !value)
    return std::unexpected(value.error());
  else if (number_result || (lhs.kind == Number && rhs.kind == Number)) return ScriptValue{Number, *value};
  // An operation between two addresses is a number,
  else if (lhs.kind != Number && rhs.kind != Number && context.current) return ScriptValue{Number, *value};
  else return ScriptValue{Absolute, *value};
}

std::expected<pepp::link::ScriptValue, pepp::tc::expr::EvaluationError>
pepp::link::evaluate(const tc::expr::Tree &tree, const ScriptContext &context) {
  if (tree.empty()) return std::unexpected(expr::EvaluationError(std::nullopt, expr::NullaryError::Evaluation_Empty));
  // Nodes are in postorder, so each node's operands are evaluated before it.
  std::vector<std::expected<ScriptValue, expr::EvaluationError>> values;
  values.reserve(tree.nodes().size());
  for (expr::NodeId id = 0; id < tree.nodes().size(); ++id)
    values.push_back(std::visit(ExpresionEvalutionVisitor{context, values, id}, tree[id]));
  return values.back();
}
