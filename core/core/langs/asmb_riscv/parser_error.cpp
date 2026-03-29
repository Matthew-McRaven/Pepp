#include "core/langs/asmb_riscv/parser_error.hpp"
#include <string>
#include "core/macros.hpp"
#include "fmt/format.h"

const std::string pepp::tc::ParserError::to_string(NullaryError err) {
  switch (err) {
  case NullaryError::Token_MissingNewline: return "Expected \\n";
  case NullaryError::Token_MissingComma: return "Expected ','";
  case NullaryError::Argument_ExpectedRD: return "Expected register rd";
  case NullaryError::Argument_ExpectedRS1: return "Expected register rs1";
  case NullaryError::Argument_ExpectedRS2: return "Expected register rs2";
  }
  PEPP_UNREACHABLE();
}

const std::string pepp::tc::ParserError::to_string(UnaryError err, std::string &arg) {
  switch (err) {
  case UnaryError::Token_Invalid: return fmt::format("Unrecognized token: {}", arg);
  }
  PEPP_UNREACHABLE();
}

pepp::tc::ParserError::ParserError(NullaryError err, support::LocationInterval loc)
    : std::logic_error(to_string(err)), loc(loc) {}

pepp::tc::ParserError::ParserError(UnaryError err, std::string arg1, support::LocationInterval loc)
    : std::logic_error(to_string(err, arg1)), loc(loc) {}
