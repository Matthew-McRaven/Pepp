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

#include <stdexcept>
#include <string>
#include "core/compile/source/location.hpp"
namespace pepp::tc {
class RISCVParserError final : public std::logic_error {
public:
  enum class NullaryError {
    Argument_ExpectedRD,
    Argument_ExpectedRS1,
    Argument_ExpectedRS2,
    Argument_ExpectedIdentNumeric,
    Argument_ExpectedImm,
    Argument_ExpectedInteger,
    Argument_ExpectedIdentifier,
    Argument_ExpectedHex,
    Argument_ExpectedString,
    Argument_Exceeded4Bytes,
    Argument_ExpectedFenceOrdering,
    Argument_SymbolicEquate,
    Argument_Missing,
    Section_StringName,
    Section_TwoArgs,
    Section_StringFlags,
    SymbolDeclaration_Required,
    SymbolDeclaration_Forbidden,
    Token_MissingNewline,
    Token_MissingComma,
    Conditional_UnmatchedEndif,
    Conditional_UnmatchedElseif,
    Conditional_UnmatchedElse,
    Conditional_MultipleElse,
    Conditional_Unterminated,
    Conditional_NotConstant,
    Macro_Unterminated,
    Macro_UnmatchedEndm,
    Macro_ExcessiveRecursion,
  };
  enum class UnaryError {
    Token_Invalid,
    Dot_Invalid,
    Expression_Invalid,
    Macro_Redefinition,
    Macro_WrongArgumentCount,
    Dot_UserError,
  };

  static const std::string to_string(NullaryError);
  static const std::string to_string(UnaryError, std::string &arg);
  explicit RISCVParserError(NullaryError err, pepp::tc::support::LocationInterval);
  explicit RISCVParserError(UnaryError err, std::string arg1, pepp::tc::support::LocationInterval);
  const pepp::tc::support::LocationInterval loc;
};
} // namespace pepp::tc
