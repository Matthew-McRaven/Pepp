#pragma once

#include <stdexcept>
#include "core/compile/source/location.hpp"
namespace pepp::tc {
class ParserError final : public std::logic_error {
public:
  enum class NullaryError {
    Argument_InvalidIntegerFormat,
    Argument_Exceeded2Bytes,
    Argument_Exceeded1Byte,
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
  };
  enum class UnaryError {
    Mnemonic_Invalid,
    AddressingMode_InvalidForMnemonic,
    Dot_Invalid,
    Token_Invalid,
  };

  static const std::string to_string(NullaryError);
  static const std::string to_string(UnaryError, std::string &arg);
  explicit ParserError(NullaryError err, pepp::tc::support::LocationInterval);
  explicit ParserError(UnaryError err, std::string arg1, pepp::tc::support::LocationInterval);
  const pepp::tc::support::LocationInterval loc;
};
} // namespace pepp::tc
