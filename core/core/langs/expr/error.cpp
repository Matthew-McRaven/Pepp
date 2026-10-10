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
#include "core/langs/expr/error.hpp"
#include "core/macros.hpp"
#include "fmt/format.h"

std::string pepp::tc::expr::to_string(NullaryError err) {
  switch (err) {
  case NullaryError::Syntax_ExpectedOperand: return "Expected an operand";
  case NullaryError::Syntax_ExpectedOpenParen: return "Expected '('";
  case NullaryError::Syntax_ExpectedCloseParen: return "Expected ')'";
  case NullaryError::Syntax_InvalidCharacter: return "Invalid character constant";
  case NullaryError::Syntax_MemberAccess: return "Member access is not implemented";
  case NullaryError::Syntax_BufferedToken: return "Unexpected token before an expression";
  case NullaryError::LocationCounter_Unavailable: return "The location counter is not available";
  case NullaryError::LocationCounter_NoValue: return "The location counter has no value";
  case NullaryError::Evaluation_Empty: return "Empty expression";
  case NullaryError::Evaluation_DivisionByZero: return "Division by zero";
  case NullaryError::Evaluation_SignedDivisionOverflow: return "Signed division overflow";
  case NullaryError::Evaluation_ShiftOutOfRange: return "Shift amount out of range";
  case NullaryError::Evaluation_UnknownOperator: return "Unknown operator";
  }
  PEPP_UNREACHABLE();
}

std::string pepp::tc::expr::to_string(UnaryError err, std::string_view arg) {
  switch (err) {
  case UnaryError::Function_Unknown: return fmt::format("Unknown function {}", arg);
  case UnaryError::Function_NotConstExpr: return fmt::format("{} cannot be evaluated here", arg);
  case UnaryError::Symbol_NoValue: return fmt::format("Symbol has no value: {}", arg);
  }
  PEPP_UNREACHABLE();
}

std::string pepp::tc::expr::to_string(const ErrorCode &code, std::string_view arg) {
  if (const auto *nullary = std::get_if<NullaryError>(&code)) return to_string(*nullary);
  return to_string(std::get<UnaryError>(code), arg);
}

pepp::tc::expr::EvaluationError::EvaluationError(std::optional<NodeId> node, NullaryError err)
    : node(node), code(err) {}

pepp::tc::expr::EvaluationError::EvaluationError(std::optional<NodeId> node, UnaryError err, std::string_view arg)
    : node(node), code(err), argument(arg) {}

pepp::tc::expr::Error::Error(support::LocationInterval location, NullaryError err)
    : location(location), code(err) {}

pepp::tc::expr::Error::Error(support::LocationInterval location, UnaryError err, std::string_view arg)
    : location(location), code(err), argument(arg) {}

pepp::tc::expr::Error::Error(support::LocationInterval location, const EvaluationError &err)
    : location(location), code(err.code), argument(err.argument) {}
