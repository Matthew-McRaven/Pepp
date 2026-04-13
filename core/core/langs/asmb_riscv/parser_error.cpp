#include "core/langs/asmb_riscv/parser_error.hpp"
#include <string>
#include "core/macros.hpp"
#include "fmt/format.h"

const std::string pepp::tc::RISCVParserError::to_string(NullaryError err) {
  switch (err) {
  case NullaryError::Token_MissingNewline: return "Expected \\n";
  case NullaryError::Token_MissingComma: return "Expected ','";
  case NullaryError::Argument_ExpectedRD: return "Expected register rd";
  case NullaryError::Argument_ExpectedRS1: return "Expected register rs1";
  case NullaryError::Argument_ExpectedRS2: return "Expected register rs2";
  case NullaryError::Argument_ExpectedIdentNumeric: return "Expected identifier or numeric argument";
  case NullaryError::Argument_ExpectedImm: return "Expected immediate operand";
  case NullaryError::Argument_InvalidIntegerFormat: return "Invalid integer format";
  case NullaryError::Argument_ExpectedInteger: return "Expected integer argument";
  case NullaryError::Argument_ExpectedHex: return "Expected hexadecimal argument";
  case NullaryError::Argument_ExpectedString: return "Expected string argument";
  case NullaryError::Argument_Exceeded1Byte: return "Argument exceeds 1 byte";
  case NullaryError::Argument_Exceeded2Bytes: return "Argument exceeds 2 bytes";
  case NullaryError::Argument_Exceeded4Bytes: return "Argument exceeds 4 bytes";
  case NullaryError::Section_StringName: return "Expected section name string";
  case NullaryError::Section_TwoArgs: return "Section directive takes exactly two arguments";
  case NullaryError::Section_StringFlags: return "Expected section flags string";
  case NullaryError::SymbolDeclaration_Required: return "Expected symbol declaration";
  case NullaryError::SymbolDeclaration_Forbidden: return "Unexpected symbol declaration";
  case NullaryError::Token_MissingLParen: return "Expected '('";
  case NullaryError::Token_MissingRParen: return "Expected ')'";
  }
  PEPP_UNREACHABLE();
}

const std::string pepp::tc::RISCVParserError::to_string(UnaryError err, std::string &arg) {
  switch (err) {
  case UnaryError::Token_Invalid: return fmt::format("Unrecognized token: {}", arg);
  case UnaryError::Dot_Invalid: return fmt::format("Invalid pseudo-operation \"{}\".", arg);
  }
  PEPP_UNREACHABLE();
}

pepp::tc::RISCVParserError::RISCVParserError(NullaryError err, support::LocationInterval loc)
    : std::logic_error(to_string(err)), loc(loc) {}

pepp::tc::RISCVParserError::RISCVParserError(UnaryError err, std::string arg1, support::LocationInterval loc)
    : std::logic_error(to_string(err, arg1)), loc(loc) {}
