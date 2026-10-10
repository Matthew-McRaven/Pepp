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
#include <span>
#include <string_view>
#include "core/integers.h"
#include "core/langs/expr/value.hpp"

namespace pepp::tc::expr {
enum class NullaryError : u8; // See error.hpp

// Unary functions such as RISC-V %hi(sym).
struct Function {
  std::string_view name;
  // Constexpr functions depend only on their argument, and are eligible for constant folding.
  // Non-constexpr functions do not yet an evaluation path yet, but depend on the execution environment.
  std::expected<Value, NullaryError> (*evaluate)(Value argument, Type default_type) = nullptr;
  // The type of the result, given the argument's.
  Type (*result_type)(Type argument, Type default_type) = nullptr;
  constexpr bool is_constexpr() const { return evaluate != nullptr; }
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
