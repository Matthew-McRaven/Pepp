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
#include <expected>
#include <functional>
#include <optional>
#include <string>
#include <vector>
#include "core/integers.h"
#include "core/langs/expr/error.hpp"
#include "core/langs/expr/ir.hpp"
#include "core/langs/expr/value.hpp"

// Utilities for evaluating expressions shared between multiple interpeters (e.g., the assembler's and the debugger's).
// Values remain untyped (bits) until an operation requires a signedness (division, negation, shifts). If one operange
// has a signedness and the other is bits, the bits takes the signedness of its partner. Otherwise, the default
// signedness is used. C's integer promotion and arithmetic rules apply when operands have different widths or
// signedness. All arithmetic occurs mod(2^size).
namespace pepp::tc::expr {

struct Options {
  // Width of the target's int
  u8 int_bits = 32;
  // By default, should bits be intepreted as signed or unsigned quantities?
  Signedness default_sign = Signedness::Signed;
};

// Evaluate integer constants
Value literal(const Integer &integer, const Options &options);
Value literal(const Character &character, const Options &options);

Type result_type(UnaryOp op, Type operand, const Options &options);
Type result_type(BinaryOp op, Type lhs, Type rhs, const Options &options);

// Compute the type-correct result for a single node's operation. Errors are returned on:
// - division by zero
// - signed division overflow (INT_MIN / -1)
// - shift amounts >= int_bits or <= 0.
std::expected<Value, std::string> apply(UnaryOp op, Value operand, const Options &options);
std::expected<Value, std::string> apply(BinaryOp op, Value lhs, Value rhs, const Options &options);

// A symbol's value in the current context or nullopt if it has none.
using ValueOf = std::function<std::optional<Value>(const Identifier &)>;

// Evaluate a tree with short-circuiting for && and ||. An identifier which value_of does not give a value (any, if
// there is no value_of) is an error.
std::expected<Value, EvaluationError> evaluate_expression(const Tree &tree, const Options &options,
                                                          const ValueOf &value_of = {});

// The type of a symbol which is not a constant.
using TypeOf = std::function<Type(const Identifier &)>;

// The value of each node, indexed by NodeId, or nullopt if the node depends on a non-constant symbol or its operation
// failed. Unlike evaluate_expression, && and || do not short-circuit.
std::vector<std::optional<Value>> constant_values(const Tree &tree, const Options &options,
                                                  const ValueOf &constant_of = {});
// The type of each node, indexed by NodeId. Constant symbols have their value's type, and other symbols type_of's.
std::vector<Type> node_types(const Tree &tree, const Options &options, const TypeOf &type_of,
                             const ValueOf &constant_of = {});

// Return a copy of the tree where each chain of +/-, *, &, |, or ^ has its constant operands moved before its other
// operands, so that fold_constants can compute them. For example, 4 * 6 + symbol - 5 becomes 4 * 6 - 5 + symbol. A chain
// is only reordered when all of its nodes have the same width.
Tree reassociate_constants(const Tree &tree, const Options &options, const TypeOf &type_of,
                           const ValueOf &constant_of = {});
// Return a copy of the tree where all expressions involving constants have been pre-computed. For example, 4 * 6 +
// symbol - 5 would become 24 + symbol - 5. Newly created constants are of type FoldedConstant which record their
// computed type in addition to their bit pattern.
Tree fold_constants(const Tree &tree, const Options &options, const ValueOf &constant_of = {});

// Reassociate before constant folding so that each chain collapses its constants into one operand. For example,
// 4 * 6 + symbol - 5 becomes 19 + symbol.  This method is useful for the assembler to prove equivalence between
// expressions and patterns which can be relocated.
Tree simplify(const Tree &tree, const Options &options, const TypeOf &type_of, const ValueOf &constant_of = {});

} // namespace pepp::tc::expr
