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

#pragma once
#include <expected>
#include <functional>
#include "core/integers.h"
#include "core/langs/expr/error.hpp"
#include "core/langs/expr/ir.hpp"
#include "core/langs/ldscript/ir.hpp"

namespace pepp::link {
using tc::ldscript::OutputSectionID;

// Represents the value of a linker script expression. ld treats numbers, absolute addresses, and addresses relative to
// an output section differently (see "The Section of an Expression" in the ld manual).
struct ScriptValue {
  enum class Kind : u8 { Number, Absolute, Relative } kind = Kind::Number;
  u64 value = 0;
  OutputSectionID section;
  bool operator==(const ScriptValue &) const = default;
};

// Represents the state to which a linker script expression may refer.
struct ScriptContext {
  // The address of the location counter.
  u64 dot = 0;
  // The output section being laid out, if any. Within it, the location counter is relative to it.
  OutputSectionID current;
  // Returns the address of an output section that has already been assigned.
  std::function<u64(OutputSectionID)> address_of;
};

// Returns the location counter, which is relative to the current output section.
ScriptValue location_counter(const ScriptContext &context);
// Returns the absolute address of a value, with numbers treated as addresses.
u64 absolute(const ScriptValue &value, const ScriptContext &context);
// Apply a binary operator, choosing the result.
std::expected<ScriptValue, tc::expr::NullaryError> combine(tc::expr::BinaryOp op, ScriptValue lhs, ScriptValue rhs,
                                                           const ScriptContext &context);
// Evaluate a linker script expression. A symbol reference is Symbol_NoValue, since symbols have no values yet.
std::expected<ScriptValue, tc::expr::EvaluationError> evaluate(const tc::expr::Tree &tree,
                                                               const ScriptContext &context);

} // namespace pepp::link
