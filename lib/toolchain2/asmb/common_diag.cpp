#include "common_diag.hpp"
#include <string>
#include "fmt/format.h"

const std::string pepp::tc::ParserError::to_string(NullaryError err) {
  switch (err) {
  case NullaryError::Argument_InvalidIntegerFormat: return "Unrecognized integer format";
  case NullaryError::Argument_Exceeded2Bytes: return "Argument must fit in two bytes";
  case NullaryError::Argument_Exceeded1Byte: return "Argument must fit in one byte";
  case NullaryError::Argument_Missing: return "Expected an argument";
  case NullaryError::Argument_ExpectedPowerOfTwo: return "Argument must be (1|2|4|8)";
  case NullaryError::Argument_ExpectedString: return "Argument must be a string";
  case NullaryError::Argument_ExpectedIdentifier: return "Argument must be an identifier";
  case NullaryError::Argument_ExpectedHex: return "Argument must be a hexadecimal integer";
  case NullaryError::AddressingMode_Required: return "Instruction requires addressin mode";
  case NullaryError::AddressingMode_Invalid: return "Invalid addressing mode";
  case NullaryError::AddressingMode_Missing: return "Expected addressing mode";
  case NullaryError::SymbolDeclaration_Required: return "Reguires a symbol declaration";
  case NullaryError::SymbolDeclaration_TooLong: return "Symbol declaration too long";
  case NullaryError::Section_StringName: return ".SECTION name must be a string";
  case NullaryError::Section_TwoArgs: return ".SECTION requires two arguments";
  case NullaryError::Section_StringFlags: return ".SECTION flags must be a string";
  case NullaryError::Token_MissingNewline: return "Expected \\n";
  }
}

const std::string pepp::tc::ParserError::to_string(UnaryError err, std::string &arg) {
  switch (err) {
  case UnaryError::Mnemonic_Invalid: return fmt::format("Invalid mnemonic \"{}\"", arg);
  case UnaryError::AddressingMode_InvalidForMnemonic:
    return fmt::format("Illegal addressing mode \"{}\"for instruction", arg);
  case UnaryError::Dot_Invalid: return fmt::format("Invalid pseudo-operation \"{}\"", arg);
  case UnaryError::Token_Invalid: return fmt::format("Unrecognized token: {}", arg);
  }
}

pepp::tc::ParserError::ParserError(NullaryError err, support::LocationInterval loc)
    : std::logic_error(to_string(err)), loc(loc) {}

pepp::tc::ParserError::ParserError(UnaryError err, std::string arg1, support::LocationInterval loc)
    : std::logic_error(to_string(err, arg1)), loc(loc) {}
