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
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include "core/compile/source/location.hpp"
#include "core/integers.h"
#include "core/langs/expr/ir.hpp"

namespace pepp::tc::expr {

enum class NullaryError : u8 {
  Syntax_ExpectedOperand,
  Syntax_ExpectedOpenParen,
  Syntax_ExpectedCloseParen,
  Syntax_InvalidCharacter,
  Syntax_MemberAccess,
  Syntax_BufferedToken,
  LocationCounter_Unavailable,
  LocationCounter_NoValue,
  Evaluation_Empty,
  Evaluation_DivisionByZero,
  Evaluation_SignedDivisionOverflow,
  Evaluation_ShiftOutOfRange,
  Evaluation_UnknownOperator,
};
// The argument names the offending symbol or function.
enum class UnaryError : u8 {
  Function_Unknown,
  Function_NotConstExpr,
  Symbol_NoValue,
};
using ErrorCode = std::variant<NullaryError, UnaryError>;

std::string to_string(NullaryError err);
std::string to_string(UnaryError err, std::string_view arg);
std::string to_string(const ErrorCode &code, std::string_view arg);

// An operation which failed during evaluation. Callers should map the node to a source location.
struct EvaluationError {
  EvaluationError(std::optional<NodeId> node, NullaryError err);
  EvaluationError(std::optional<NodeId> node, UnaryError err, std::string_view arg);
  // True if this is the error code with the given argument.
  bool matches(ErrorCode code, std::string_view argument = {}) const {
    return this->code == code && this->argument == argument;
  }
  std::optional<NodeId> node; // nullopt if the tree is empty.
  ErrorCode code;
  std::string argument;
  std::string message() const { return to_string(code, argument); }
};

// A problem found while parsing an expression and the location in the lexer's buffer where it occured.
struct Error {
  Error(support::LocationInterval location, NullaryError err);
  Error(support::LocationInterval location, UnaryError err, std::string_view arg);
  // An evaluation error, placed at the source of its node.
  Error(support::LocationInterval location, const EvaluationError &err);
  // True if this is the error code with the given argument.
  bool matches(ErrorCode code, std::string_view argument = {}) const {
    return this->code == code && this->argument == argument;
  }
  support::LocationInterval location;
  ErrorCode code;
  std::string argument;
  std::string message() const { return to_string(code, argument); }
};

} // namespace pepp::tc::expr
