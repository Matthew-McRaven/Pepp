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

// Evaluate a tree containing no identifiers with short-circuiting for && and ||.
// A tree containing an identifier will raise an error.
std::expected<Value, Error> evaluate_constant(const Tree &tree, const Options &options);
} // namespace pepp::tc::expr
