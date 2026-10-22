/*
 * Copyright (c) 2025-2026 J. Stanley Warford, Matthew McRaven
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
class PepRecursionError final : public std::logic_error {
public:
  explicit PepRecursionError(pepp::tc::support::LocationInterval ival);
  const pepp::tc::support::LocationInterval loc;
};

class PepParserError final : public std::logic_error {
public:
  enum class NullaryError {
    Argument_InvalidIntegerFormat,
    Argument_SymbolicEquate,
    Argument_Exceeded2Bytes,
    Argument_Missing,
    Argument_ExpectedPowerOfTwo,
    Argument_ExpectedString,
    Argument_ExpectedIdentifier,
    Argument_ExpectedHex,
    Argument_ExpectedInteger,
    AddressingMode_Required,
    AddressingMode_Invalid,
    AddressingMode_Missing,
    SymbolDeclaration_Required,
    SymbolDeclaration_Forbidden,
    SymbolDeclaration_TooLong,
    Section_StringName,
    Section_TwoArgs,
    Section_StringFlags,
    Token_MissingNewline,
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
    Mnemonic_Invalid,
    AddressingMode_InvalidForMnemonic,
    Dot_Invalid,
    Token_Invalid,
    Macro_Redefinition,
    Macro_WrongArgumentCount,
    Expression_Invalid,
  };

  static const std::string to_string(NullaryError);
  static const std::string to_string(UnaryError, const std::string &arg);
  explicit PepParserError(NullaryError err, pepp::tc::support::LocationInterval);
  explicit PepParserError(UnaryError err, std::string arg1, pepp::tc::support::LocationInterval);
  const pepp::tc::support::LocationInterval loc;
};
} // namespace pepp::tc
