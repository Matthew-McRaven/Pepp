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
#include <algorithm>
#include <utility>
#include "core/langs/expr/traversal.hpp"
#include "core/math/bitmanip/mask.hpp"

namespace {
using namespace pepp::tc::expr;
using Integer = pepp::tc::expr::Integer;
using enum Signedness;

// How to read t's bits when an operator needs a sign.
Signedness resolve_typeof_bits(Type t, Type other, const Options &options) {
  if (t.sign != Bits) return t.sign;
  if (other.sign != Bits) return other.sign;
  return options.default_sign;
}

Type int_type(const Options &options) { return {options.int_bits, Signed}; }

// C's integer promotion rules. Anything narrower than int becomes a (signed) int.
// If the value is (untyped) bits, then infer the sign from its partner or the default.
Type promote(Type t, Type other, const Options &options) {
  const auto sign = resolve_typeof_bits(t, other, options);
  if (t.bits >= options.int_bits) return {t.bits, sign};
  return {options.int_bits, t.sign == Bits ? sign : Signed};
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

OperationTypes operation_types(UnaryOp op, Type t, const Options &options) {
  if (op == UnaryOp::LogicalNot) return {t, int_type(options)};
  // If input is larger than int, keep its sign. Otherwise we follow C's integer promotion rules
  const auto converted = t.bits >= options.int_bits ? t : promote(t, t, options);
  // Unary minus (signed negation) forces the result to be signed.
  const auto result = op == UnaryOp::Minus ? Type{converted.bits, Signed} : converted;
  return {converted, result};
}

OperationTypes operation_types(BinaryOp op, Type a, Type b, const Options &options) {
  using enum BinaryOp;
  // Comparisons against 0, so we can just work on ints.
  if (op == LogicalAnd || op == LogicalOr) return {int_type(options), int_type(options)};
  if (op == ShiftLeft || op == ShiftRight) {
    // Promote shift amount to be the same as the value's type. Shifts are < 64 bits, which would fit even in an i8!
    const auto lhs = op == ShiftLeft && a.bits >= options.int_bits ? a : promote(a, a, options);
    return {lhs, lhs};
  }
  // (bit x bit) inputs retain their non-signedness if they can dodge integer promotion (by being sufficiently large),
  // are the same size, and when the operator doesn't care about it's inputs' signedness.
  const bool sign_agnostic = a == b && a.sign == Bits && a.bits >= options.int_bits && !needs_sign(op);
  const auto common = sign_agnostic ? a : usual(promote(a, b, options), promote(b, a, options));
  return {common, is_comparison(op) ? int_type(options) : common};
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

Result evaluate(const Tree &tree, NodeId id, const Options &options) {
  const auto fail = [&](std::string message) { return std::unexpected(EvaluationError{id, std::move(message)}); };
  const auto f = [&](const auto &n) -> Result {
    using T = std::decay_t<decltype(n)>;
    if constexpr (std::is_same_v<T, Integer> || std::is_same_v<T, Character>) return literal(n, options);
    else if constexpr (std::is_same_v<T, FoldedConstant>) return n.value;
    else if constexpr (std::is_same_v<T, Identifier>) return fail("Symbols are not allowed in a constant expression");
    else if constexpr (std::is_same_v<T, Unary>) {
      if (const auto operand = evaluate(tree, n.operand, options); !operand) return operand;
      else if (auto ret = apply(n.op, *operand, options); ret) return *ret;
      else return fail(ret.error());
    } else {
      if (const auto lhs = evaluate(tree, n.lhs, options); !lhs) return lhs;
      else if (const auto rhs = short_circuits(n.op, *lhs) ? lhs : evaluate(tree, n.rhs, options); !rhs) return rhs;
      else if (auto ret = apply(n.op, *lhs, *rhs, options); ret) return *ret;
      else return fail(ret.error());
    }
  };
  return std::visit(f, tree[id]);
}
} // namespace

pepp::tc::expr::Value pepp::tc::expr::literal(const Integer &integer, const Options &options) {
  // Find the smallest int type which could hold the bit pattern.
  for (u8 width = options.int_bits; width < 64; width *= 2)
    if (integer.value <= bits::mask(width / 8)) return {integer.value, {width, Bits}};
  return {integer.value, {64, Bits}};
}

pepp::tc::expr::Value pepp::tc::expr::literal(const Character &character, const Options &options) {
  return {character.value, {options.int_bits, Bits}};
}

pepp::tc::expr::Type pepp::tc::expr::result_type(UnaryOp op, Type operand, const Options &options) {
  return operation_types(op, operand, options).result;
}

pepp::tc::expr::Type pepp::tc::expr::result_type(BinaryOp op, Type lhs, Type rhs, const Options &options) {
  return operation_types(op, lhs, rhs, options).result;
}

std::expected<pepp::tc::expr::Value, std::string> pepp::tc::expr::apply(UnaryOp op, Value operand,
                                                                        const Options &options) {
  const auto types = operation_types(op, operand.type, options);
  const auto v = convert(operand, types.operand);
  switch (op) {
  case UnaryOp::Plus: return v;
  case UnaryOp::Minus: return make(0 - v.bits, types.result);
  case UnaryOp::BitNot: return make(~v.bits, types.result);
  case UnaryOp::LogicalNot: return make(operand.bits == 0, types.result);
  }
  return std::unexpected("Unknown unary operator");
}

std::expected<pepp::tc::expr::Value, std::string> pepp::tc::expr::apply(BinaryOp op, Value lhs, Value rhs,
                                                                        const Options &options) {
  using enum BinaryOp;
  const auto types = operation_types(op, lhs.type, rhs.type, options);
  const auto l = convert(lhs, types.operand), r = convert(rhs, types.operand);
  const bool is_signed = types.operand.sign == Signed;
  switch (op) {
  case Multiply: return make(l.bits * r.bits, types.result);
  case Divide: [[fallthrough]];
  case Modulo: {
    if (r.bits == 0) return std::unexpected("Division by zero");
    if (!is_signed) return make(op == Divide ? l.bits / r.bits : l.bits % r.bits, types.result);
    const i64 x = l.as_signed(), y = r.as_signed();
    if (y == -1 && l.bits == (bits::mask(l.type.bits / 8) >> 1) + 1) return std::unexpected("Signed division overflow");
    return make(static_cast<u64>(op == Divide ? x / y : x % y), types.result);
  }
  case Add: return make(l.bits + r.bits, types.result);
  case Subtract: return make(l.bits - r.bits, types.result);
  case ShiftLeft: [[fallthrough]];
  case ShiftRight: {
    // Actually need to treat shift amount as unsigned. negative shift amount is nonsense.
    if (const u64 count = rhs.bits; count >= types.result.bits) return std::unexpected("Shift amount out of range");
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
  return std::unexpected("Unknown binary operator");
}

std::expected<pepp::tc::expr::Value, pepp::tc::expr::EvaluationError>
pepp::tc::expr::evaluate_constant(const Tree &tree, const Options &options) {
  if (tree.empty()) return std::unexpected(EvaluationError{std::nullopt, "Empty expression"});
  return evaluate(tree, tree.root(), options);
}

std::vector<std::optional<pepp::tc::expr::Value>>
pepp::tc::expr::constant_values(const Tree &tree, const Options &options, const ConstantOf &constant_of) {
  // Nodes are in postorder, so operands are always computed first.
  std::vector<std::optional<Value>> values(tree.nodes().size());
  const auto known = [](const std::expected<Value, std::string> &v) {
    return v ? std::optional<Value>(*v) : std::nullopt;
  };
  // Evaluate a node to a value if it is constant or a nullopt if it is a symbol (or an expression containing a symbol).
  const auto f = [&](const auto &n) -> std::optional<Value> {
    using T = std::decay_t<decltype(n)>;
    if constexpr (std::is_same_v<T, Integer> || std::is_same_v<T, Character>) return literal(n, options);
    else if constexpr (std::is_same_v<T, FoldedConstant>) return n.value;
    else if constexpr (std::is_same_v<T, Identifier>) return constant_of ? constant_of(n) : std::nullopt;
    else if constexpr (std::is_same_v<T, Unary>) {
      if (!values[n.operand]) return std::nullopt;
      return known(apply(n.op, *values[n.operand], options));
    } else {
      if (const auto &lhs = values[n.lhs], &rhs = values[n.rhs]; !lhs || !rhs) return std::nullopt;
      else return known(apply(n.op, *lhs, *rhs, options));
    }
  };
  for (NodeId id = 0; id < values.size(); id++) values[id] = std::visit(f, tree[id]);
  return values;
}

std::vector<pepp::tc::expr::Type> pepp::tc::expr::node_types(const Tree &tree, const Options &options,
                                                             const TypeOf &type_of, const ConstantOf &constant_of) {
  // Nodes are in postorder, so operands are always typed first.
  std::vector<Type> types(tree.nodes().size());
  const auto f = [&](const auto &n) -> Type {
    using T = std::decay_t<decltype(n)>;
    if constexpr (std::is_same_v<T, Integer> || std::is_same_v<T, Character>) return literal(n, options).type;
    else if constexpr (std::is_same_v<T, FoldedConstant>) return n.value.type;
    else if constexpr (std::is_same_v<T, Identifier>) {
      if (const auto value = constant_of ? constant_of(n) : std::nullopt) return value->type;
      return type_of(n);
    } else if constexpr (std::is_same_v<T, Unary>) return result_type(n.op, types[n.operand], options);
    else return result_type(n.op, types[n.lhs], types[n.rhs], options);
  };
  for (NodeId id = 0; id < types.size(); id++) types[id] = std::visit(f, tree[id]);
  return types;
}

pepp::tc::expr::Tree pepp::tc::expr::fold_constants(const Tree &tree, const Options &options,
                                                    const ConstantOf &constant_of) {
  const auto values = constant_values(tree, options, constant_of);

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
