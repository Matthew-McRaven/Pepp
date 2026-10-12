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
#include <span>
#include <string_view>
#include <variant>
#include "core/compile/symbol/types.hpp"
#include "core/integers.h"
#include "core/langs/expr/value.hpp"

namespace pepp::tc::expr {
enum class NullaryError : u8; // See error.hpp

// A symbol's value in the current context as well as its definition state for use in consteval contexts.
struct EvaluatedSymbol {
  pepp::core::symbol::DefinitionState state = pepp::core::symbol::DefinitionState::Undefined;
  std::optional<Value> value = std::nullopt;
};

// An argument as the function sees it, matching its Parameter.
using Argument = std::variant<Value, EvaluatedSymbol>;

// Functions such as RISC-V's %hi(sym). The category decides when they are evaluated.
struct Function {
  enum class Parameter : u8 {
    Value,  // A sub-tree which must be evaluate()'ed befor evaluating the function
    Symbol, // An identifier argument with special handling and must not be eagerly evaluated()'ed.
  };
  enum class Category : u8 {
    ConstEval,    // Must be evaluated as it is parsed (e.g., %defined(sym)).
    ConstExpr,    // May be evaluated at parse time if its arguments are constant expressions(e.g., %hi).
    NonConstExpr, // Never evaluated at parse time (e.g., %pcrel_hi).
  };
  std::string_view name;
  Category category = Category::ConstExpr;
  // One per argument.
  std::span<const Parameter> parameters;
  // ConstEval and ConstExpr only.
  std::function<std::expected<Value, NullaryError>(std::span<const Argument> arguments, Type default_type)> evaluate;
  // The type of the result, given the arguments'. ConstExpr and NonConstExpr only.
  std::function<Type(std::span<const Type> arguments, Type default_type)> result_type;
};

// How a language reads and evaluates its expressions.
struct Options {
  // The width of the target's int, and the signedness given to bits when neither operand has one.
  Type default_type = {32, Signedness::Signed};
  // How to interpret the full-stop character?
  enum class Dot : u8 {
    Forbidden,  // Not part of the grammar.
    Identifier, // The location counter. TODO: also allowed in identifiers, e.g., .L1 (a GNU local symbol).
    Operator,   // Member access (e.g., the debugger's a.b). Not implemented yet.
  } dot = Dot::Forbidden;
  // Lex %name as an identifier, which must then name a function (e.g., RISC-V's %hi). Otherwise, % is modulo.
  bool percent_identifiers = false;
  // Functions that should be recognized by the language.
  std::span<const Function> functions;
};

} // namespace pepp::tc::expr
