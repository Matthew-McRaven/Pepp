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
#include "core/langs/expr/evaluator.hpp"
#include "core/langs/expr/options.hpp"
#include <algorithm>
#include <array>
#include <utility>
#include "core/langs/expr/traversal.hpp"
#include "core/math/bitmanip/mask.hpp"

namespace {
using namespace pepp::tc::expr;
using Integer = pepp::tc::expr::Integer;
using enum Signedness;

// How to read t's bits when an operator needs a sign.
Signedness resolve_typeof_bits(Type t, Type other, Type default_type) {
  if (t.sign != Bits) return t.sign;
  if (other.sign != Bits) return other.sign;
  return default_type.sign;
}

Type int_type(Type default_type) { return {default_type.bits, Signed}; }

// C's integer promotion rules. Anything narrower than int becomes a (signed) int.
// If the value is (untyped) bits, then infer the sign from its partner or the default.
Type promote(Type t, Type other, Type default_type) {
  const auto sign = resolve_typeof_bits(t, other, default_type);
  if (t.bits >= default_type.bits) return {t.bits, sign};
  return {default_type.bits, t.sign == Bits ? sign : Signed};
}

// C's usual arithmetic conversions on two signed or unsigned types.
Type usual(Type a, Type b) {
  if (a.sign == b.sign) return {std::max(a.bits, b.bits), a.sign};
  if (a.sign != Unsigned) std::swap(a, b); // Now a is unsigned and b signed.
  return a.bits >= b.bits ? a : b;         // A wider signed type holds every value of the unsigned one.
}

bool needs_sign(BinaryOp op) {
  using enum BinaryOp;
  switch (op) {
  case Divide: [[fallthrough]];
  case Modulo: [[fallthrough]];
  case ShiftRight: [[fallthrough]];
  case Less: [[fallthrough]];
  case LessEqual: [[fallthrough]];
  case Greater: [[fallthrough]];
  case GreaterEqual: return true;
  default: return false;
  }
}

bool is_comparison(BinaryOp op) {
  using enum BinaryOp;
  return op == Less || op == LessEqual || op == Greater || op == GreaterEqual || op == Equal || op == NotEqual;
}

struct OperationTypes {
  // The type of the input operand(s) and the type of the result.
  Type operand, result;
};

OperationTypes operation_types(UnaryOp op, Type t, Type default_type) {
  if (op == UnaryOp::LogicalNot) return {t, int_type(default_type)};
  // If input is larger than int, keep its sign. Otherwise we follow C's integer promotion rules
  const auto converted = t.bits >= default_type.bits ? t : promote(t, t, default_type);
  // Unary minus (signed negation) forces the result to be signed.
  const auto result = op == UnaryOp::Minus ? Type{converted.bits, Signed} : converted;
  return {converted, result};
}

OperationTypes operation_types(BinaryOp op, Type a, Type b, Type default_type) {
  using enum BinaryOp;
  // Comparisons against 0, so we can just work on ints.
  if (op == LogicalAnd || op == LogicalOr) return {int_type(default_type), int_type(default_type)};
  if (op == ShiftLeft || op == ShiftRight) {
    // Promote shift amount to be the same as the value's type. Shifts are < 64 bits, which would fit even in an i8!
    const auto lhs = op == ShiftLeft && a.bits >= default_type.bits ? a : promote(a, a, default_type);
    return {lhs, lhs};
  }
  // (bit x bit) inputs retain their non-signedness if they can dodge integer promotion (by being sufficiently large),
  // are the same size, and when the operator doesn't care about it's inputs' signedness.
  const bool sign_agnostic = a == b && a.sign == Bits && a.bits >= default_type.bits && !needs_sign(op);
  const auto common = sign_agnostic ? a : usual(promote(a, b, default_type), promote(b, a, default_type));
  return {common, is_comparison(op) ? int_type(default_type) : common};
}

// Widen or truncate v to the destination type with the appropriate masking or sign/0 extension.
// Bit inputs 0- or sign-extend according to the destination type's signedness
Value convert(Value v, Type to) {
  const auto sign = v.type.sign == Bits ? to.sign : v.type.sign;
  const u64 widened = sign == Signed ? bits::sign_extend(v.bits, v.type.bits / 8) : v.bits;
  return {widened & bits::mask(to.bits / 8), to};
}

Value make(u64 value, Type type) { return {value & bits::mask(type.bits / 8), type}; }

using Result = std::expected<Value, EvaluationError>;

// If this returns true, do not evaluate the RHS.
bool short_circuits(BinaryOp op, Value lhs) {
  return (op == BinaryOp::LogicalAnd && lhs.bits == 0) || (op == BinaryOp::LogicalOr && lhs.bits != 0);
}

Result evaluate(const Tree &tree, NodeId id, Type default_type, const ValueOf &value_of) {
  const auto fail = [&]<typename... Args>(Args &&...args) {
    return std::unexpected(EvaluationError{id, std::forward<Args>(args)...});
  };
  const auto f = [&](const auto &n) -> Result {
    using T = std::decay_t<decltype(n)>;
    if constexpr (std::is_same_v<T, Integer> || std::is_same_v<T, Character>) return literal(n, default_type);
    else if constexpr (std::is_same_v<T, FoldedConstant>) return n.value;
    else if constexpr (std::is_same_v<T, Identifier>) {
      if (auto value = value_of ? value_of(n) : std::nullopt) return *value;
      return fail(UnaryError::Symbol_NoValue, n.name);
    } else if constexpr (std::is_same_v<T, LocationCounter>) { // Location counter is just another symbol.
      if (auto value = value_of ? value_of(Identifier{n.name}) : std::nullopt) return *value;
      return fail(NullaryError::LocationCounter_NoValue);
    } else if constexpr (std::is_same_v<T, Parens>) return evaluate(tree, n.inner, default_type, value_of);
    else if constexpr (std::is_same_v<T, Call>) {
      if (!n.function->is_constexpr()) return fail(UnaryError::Function_NotConstExpr, n.function->name);
      else if (const auto argument = evaluate(tree, n.argument, default_type, value_of); !argument) return argument;
      else if (auto ret = n.function->evaluate(*argument, default_type); ret) return *ret;
      else return fail(ret.error());
    } else if constexpr (std::is_same_v<T, Unary>) {
      if (const auto operand = evaluate(tree, n.operand, default_type, value_of); !operand) return operand;
      else if (auto ret = apply(n.op, *operand, default_type); ret) return *ret;
      else return fail(ret.error());
    } else {
      if (const auto lhs = evaluate(tree, n.lhs, default_type, value_of); !lhs) return lhs;
      else if (const auto rhs = short_circuits(n.op, *lhs) ? lhs : evaluate(tree, n.rhs, default_type, value_of); !rhs) return rhs;
      else if (auto ret = apply(n.op, *lhs, *rhs, default_type); ret) return *ret;
      else return fail(ret.error());
    }
  };
  return std::visit(f, tree[id]);
}
} // namespace

pepp::tc::expr::Value pepp::tc::expr::literal(const Integer &integer, Type default_type) {
  // Find the smallest int type which could hold the bit pattern.
  for (u8 width = default_type.bits; width < 64; width *= 2)
    if (integer.value <= bits::mask(width / 8)) return {integer.value, {width, Bits}};
  return {integer.value, {64, Bits}};
}

pepp::tc::expr::Value pepp::tc::expr::literal(const Character &character, Type default_type) {
  return {character.value, {default_type.bits, Bits}};
}

pepp::tc::expr::Type pepp::tc::expr::result_type(UnaryOp op, Type operand, Type default_type) {
  return operation_types(op, operand, default_type).result;
}

pepp::tc::expr::Type pepp::tc::expr::result_type(BinaryOp op, Type lhs, Type rhs, Type default_type) {
  return operation_types(op, lhs, rhs, default_type).result;
}

std::expected<pepp::tc::expr::Value, pepp::tc::expr::NullaryError> pepp::tc::expr::apply(UnaryOp op, Value operand,
                                                                                          Type default_type) {
  const auto types = operation_types(op, operand.type, default_type);
  const auto v = convert(operand, types.operand);
  switch (op) {
  case UnaryOp::Plus: return v;
  case UnaryOp::Minus: return make(0 - v.bits, types.result);
  case UnaryOp::BitNot: return make(~v.bits, types.result);
  case UnaryOp::LogicalNot: return make(operand.bits == 0, types.result);
  }
  return std::unexpected(NullaryError::Evaluation_UnknownOperator);
}

std::expected<pepp::tc::expr::Value, pepp::tc::expr::NullaryError>
pepp::tc::expr::apply(BinaryOp op, Value lhs, Value rhs, Type default_type) {
  using enum BinaryOp;
  const auto types = operation_types(op, lhs.type, rhs.type, default_type);
  const auto l = convert(lhs, types.operand), r = convert(rhs, types.operand);
  const bool is_signed = types.operand.sign == Signed;
  switch (op) {
  case Multiply: return make(l.bits * r.bits, types.result);
  case Divide: [[fallthrough]];
  case Modulo: {
    if (r.bits == 0) return std::unexpected(NullaryError::Evaluation_DivisionByZero);
    if (!is_signed) return make(op == Divide ? l.bits / r.bits : l.bits % r.bits, types.result);
    const i64 x = l.as_signed(), y = r.as_signed();
    if (y == -1 && l.bits == (bits::mask(l.type.bits / 8) >> 1) + 1) return std::unexpected(NullaryError::Evaluation_SignedDivisionOverflow);
    return make(static_cast<u64>(op == Divide ? x / y : x % y), types.result);
  }
  case Add: return make(l.bits + r.bits, types.result);
  case Subtract: return make(l.bits - r.bits, types.result);
  case ShiftLeft: [[fallthrough]];
  case ShiftRight: {
    // Actually need to treat shift amount as unsigned. negative shift amount is nonsense.
    if (const u64 count = rhs.bits; count >= types.result.bits) return std::unexpected(NullaryError::Evaluation_ShiftOutOfRange);
    else if (op == ShiftLeft) return make(l.bits << count, types.result);
    else return make(is_signed ? static_cast<u64>(l.as_signed() >> count) : l.bits >> count, types.result);
  }
  case Less: return make(is_signed ? l.as_signed() < r.as_signed() : l.bits < r.bits, types.result);
  case LessEqual: return make(is_signed ? l.as_signed() <= r.as_signed() : l.bits <= r.bits, types.result);
  case Greater: return make(is_signed ? l.as_signed() > r.as_signed() : l.bits > r.bits, types.result);
  case GreaterEqual: return make(is_signed ? l.as_signed() >= r.as_signed() : l.bits >= r.bits, types.result);
  case Equal: return make(l.bits == r.bits, types.result);
  case NotEqual: return make(l.bits != r.bits, types.result);
  case BitAnd: return make(l.bits & r.bits, types.result);
  case BitXor: return make(l.bits ^ r.bits, types.result);
  case BitOr: return make(l.bits | r.bits, types.result);
  case LogicalAnd: return make(lhs.bits != 0 && rhs.bits != 0, types.result);
  case LogicalOr: return make(lhs.bits != 0 || rhs.bits != 0, types.result);
  }
  return std::unexpected(NullaryError::Evaluation_UnknownOperator);
}

std::expected<pepp::tc::expr::Value, pepp::tc::expr::EvaluationError>
pepp::tc::expr::evaluate_expression(const Tree &tree, Type default_type, const ValueOf &value_of) {
  if (tree.empty()) return std::unexpected(EvaluationError{std::nullopt, NullaryError::Evaluation_Empty});
  return evaluate(tree, tree.root(), default_type, value_of);
}

std::vector<std::optional<pepp::tc::expr::Value>>
pepp::tc::expr::constant_values(const Tree &tree, Type default_type, const ValueOf &constant_of) {
  // Nodes are in postorder, so operands are always computed first.
  std::vector<std::optional<Value>> values(tree.nodes().size());
  const auto known = [](const std::expected<Value, NullaryError> &v) {
    return v ? std::optional<Value>(*v) : std::nullopt;
  };
  // Evaluate a node to a value if it is constant or a nullopt if it is a symbol (or an expression containing a symbol).
  const auto f = [&](const auto &n) -> std::optional<Value> {
    using T = std::decay_t<decltype(n)>;
    if constexpr (std::is_same_v<T, Integer> || std::is_same_v<T, Character>) return literal(n, default_type);
    else if constexpr (std::is_same_v<T, FoldedConstant>) return n.value;
    else if constexpr (std::is_same_v<T, Identifier>) return constant_of ? constant_of(n) : std::nullopt;
    else if constexpr (std::is_same_v<T, LocationCounter>) return std::nullopt;
    else if constexpr (std::is_same_v<T, Parens>) return values[n.inner];
    // A constexpr call is constant when its argument is.
    else if constexpr (std::is_same_v<T, Call>) {
      if (!n.function->is_constexpr() || !values[n.argument]) return std::nullopt;
      return known(n.function->evaluate(*values[n.argument], default_type));
    }
    else if constexpr (std::is_same_v<T, Unary>) {
      if (!values[n.operand]) return std::nullopt;
      return known(apply(n.op, *values[n.operand], default_type));
    } else {
      if (const auto &lhs = values[n.lhs], &rhs = values[n.rhs]; !lhs || !rhs) return std::nullopt;
      else return known(apply(n.op, *lhs, *rhs, default_type));
    }
  };
  for (NodeId id = 0; id < values.size(); id++) values[id] = std::visit(f, tree[id]);
  return values;
}

std::vector<pepp::tc::expr::Type> pepp::tc::expr::node_types(const Tree &tree, Type default_type,
                                                             const TypeOf &type_of, const ValueOf &constant_of) {
  // Nodes are in postorder, so operands are always typed first.
  std::vector<Type> types(tree.nodes().size());
  const auto f = [&](const auto &n) -> Type {
    using T = std::decay_t<decltype(n)>;
    if constexpr (std::is_same_v<T, Integer> || std::is_same_v<T, Character>) return literal(n, default_type).type;
    else if constexpr (std::is_same_v<T, FoldedConstant>) return n.value.type;
    else if constexpr (std::is_same_v<T, Identifier>) {
      if (const auto value = constant_of ? constant_of(n) : std::nullopt) return value->type;
      return type_of(n);
    } else if constexpr (std::is_same_v<T, LocationCounter>) return type_of(Identifier{n.name});
    else if constexpr (std::is_same_v<T, Parens>) return types[n.inner];
    else if constexpr (std::is_same_v<T, Call>) return n.function->result_type(types[n.argument], default_type);
    else if constexpr (std::is_same_v<T, Unary>) return result_type(n.op, types[n.operand], default_type);
    else return result_type(n.op, types[n.lhs], types[n.rhs], default_type);
  };
  for (NodeId id = 0; id < types.size(); id++) types[id] = std::visit(f, tree[id]);
  return types;
}

pepp::tc::expr::Tree pepp::tc::expr::reassociate_constants(const Tree &tree, Type default_type,
                                                           const TypeOf &type_of, const ValueOf &constant_of) {
  using namespace bits;
  using enum BinaryOp;
  // && and || are associative too, but moving a constant ahead of them would change what short-circuiting evaluates.
  static constexpr std::array<Group, 5> groups{{
      {Add, Subtract, 0},
      {Multiply, std::nullopt, 1},
      {BitAnd, std::nullopt, ~u64{0}},
      {BitOr, std::nullopt, 0},
      {BitXor, std::nullopt, 0},
  }};
  const auto types = node_types(tree, default_type, type_of, constant_of);
  const auto values = constant_values(tree, default_type, constant_of);

  // Rebuild from the root, left to right so that the output stays in postorder.
  Tree ret;
  const auto rebuild = [&](const auto &self, NodeId id) -> NodeId {
    const auto copy = [&](NodeId operand) { return self(self, operand); };
    // Does the current tree node match any of the groups that we know how to re-associate?
    const auto group = std::ranges::find_if(groups, [&](const Group &g) { return any(tree.kinds()[id] & g.kinds()); });
    // If not, then just emit the sub-tree to the output
    if (group == groups.end() || values[id]) return ret.add(map_operands(tree[id], copy));

    // Recurse through our children to extract all of the sub-expressions which share our operator (or it's inverse)
    auto terms = flatten(tree, id, *group);
    // Don't attempt to re-associate if the operands are of different sizes, since that could change the final result.
    const auto same_width = [&](const ChainOperand &term) { return types[term.id].bits == types[id].bits; };
    if (!std::ranges::all_of(terms, same_width)) return ret.add(map_operands(tree[id], copy));

    // Emit constants, then inverted constants, then everything else, each in source order.
    const auto rank = [&](const ChainOperand &term) { return values[term.id] ? (term.inverted ? 1 : 0) : 2; };
    std::ranges::stable_sort(terms, {}, rank);
    std::optional<NodeId> acc;
    // Prefer to emit an identity constant if the chain would otherwise start with an inverted term, which can be
    // eliminated by constant folding.
    if (terms.front().inverted) acc = ret.add(FoldedConstant{make(group->identity, types[id])});
    for (const auto &term : terms) {
      // Apply rebuild to our operands (from left to right)
      const auto operand = copy(term.id);
      // Join existing values into a tree combined via the group's operator.
      acc = acc ? ret.add(Binary{term.inverted ? *group->inverse : group->op, *acc, operand}) : operand;
    }
    // Return the fully re-associated tree to the caller.
    return *acc;
  };
  if (!tree.empty()) rebuild(rebuild, tree.root());
  return ret;
}

pepp::tc::expr::Tree pepp::tc::expr::fold_constants(const Tree &tree, Type default_type,
                                                    const ValueOf &constant_of) {
  const auto values = constant_values(tree, default_type, constant_of);

  // Rebuild from the root, replacing each subtree which has a value with a single constant.
  Tree ret;
  const auto rebuild = [&](const auto &self, NodeId id) -> NodeId {
    const auto &node = tree[id];
    if (!values[id]) return ret.add(map_operands(node, [&](NodeId operand) { return self(self, operand); }));
    // Do not rewrite a single literal as a folded constant.
    if (std::holds_alternative<Integer>(node) || std::holds_alternative<Character>(node)) return ret.add(node);
    return ret.add(FoldedConstant{*values[id]});
  };
  if (!tree.empty()) rebuild(rebuild, tree.root());
  return ret;
}

pepp::tc::expr::Tree pepp::tc::expr::simplify(const Tree &tree, Type default_type, const TypeOf &type_of,
                                              const ValueOf &constant_of) {
  return fold_constants(reassociate_constants(strip_parens(tree), default_type, type_of, constant_of), default_type, constant_of);
}
