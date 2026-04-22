#include "core/langs/asmb_pep/parser_error.hpp"
#include <string>
#include "core/macros.hpp"
#include "fmt/format.h"

const std::string pepp::tc::PepParserError::to_string(NullaryError err) {
  switch (err) {
  case NullaryError::Argument_InvalidIntegerFormat: return "Unrecognized integer format";
  case NullaryError::Argument_Exceeded2Bytes: return "Argument must fit in two bytes";
  case NullaryError::Argument_Exceeded1Byte: return "Argument must fit in one byte";
  case NullaryError::Argument_Missing: return "Expected an argument";
  case NullaryError::Argument_ExpectedPowerOfTwo: return "Argument must be (1|2|4|8)";
  case NullaryError::Argument_ExpectedString: return "Argument must be a string";
  case NullaryError::Argument_ExpectedIdentifier: return "Argument must be an identifier";
  case NullaryError::Argument_ExpectedHex: return "Argument must be a hexadecimal integer";
  case NullaryError::Argument_ExpectedInteger: return "Argument must be an integer";
  case NullaryError::AddressingMode_Required: return "Addressing mode required for this instruction.";
  case NullaryError::AddressingMode_Invalid: return "Invalid addressing mode";
  case NullaryError::AddressingMode_Missing: return "Expected addressing mode";
  case NullaryError::SymbolDeclaration_Required: return "Requires a symbol declaration";
  case NullaryError::SymbolDeclaration_Forbidden: return "Does not allow a symbol declaration";
  case NullaryError::SymbolDeclaration_TooLong: return "Symbol declaration too long";
  case NullaryError::Section_StringName: return ".SECTION name must be a string";
  case NullaryError::Section_TwoArgs: return ".SECTION requires two arguments";
  case NullaryError::Section_StringFlags: return ".SECTION flags must be a string";
  case NullaryError::Token_MissingNewline: return "Expected \\n";
  case NullaryError::Conditional_UnmatchedEndif: return "Unmatched .ENDIF directive";
  case NullaryError::Conditional_Unterminated: return "Unterminated conditional directive";
  case NullaryError::Conditional_UnmatchedElseif: return "Unmatched .ELSEIF directive";
  case NullaryError::Conditional_UnmatchedElse: return "Unmatched .ELSE directive";
  case NullaryError::Conditional_MultipleElse: return "Multiple .ELSE directives in the same conditional";
  case NullaryError::Macro_Unterminated: return "Unterminated macro definition";
  }
  PEPP_UNREACHABLE();
}

const std::string pepp::tc::PepParserError::to_string(UnaryError err, std::string &arg) {
  switch (err) {
  case UnaryError::Mnemonic_Invalid: return fmt::format("Invalid mnemonic \"{}\".", arg);
  case UnaryError::AddressingMode_InvalidForMnemonic:
    return fmt::format("Illegal addressing mode \"{}\" for this instruction", arg);
  case UnaryError::Dot_Invalid: return fmt::format("Invalid pseudo-operation \"{}\".", arg);
  case UnaryError::Token_Invalid: return fmt::format("Unrecognized token: {}", arg);
  }
  PEPP_UNREACHABLE();
}

pepp::tc::PepParserError::PepParserError(NullaryError err, support::LocationInterval loc)
    : std::logic_error(to_string(err)), loc(loc) {}

pepp::tc::PepParserError::PepParserError(UnaryError err, std::string arg1, support::LocationInterval loc)
    : std::logic_error(to_string(err, arg1)), loc(loc) {}
