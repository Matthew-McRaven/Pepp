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
    Argument_InvalidIntegerFormat,
    Token_MissingNewline,
    Token_MissingComma,
    Token_MissingLParen,
    Token_MissingRParen,
  };
  enum class UnaryError { Token_Invalid };

  static const std::string to_string(NullaryError);
  static const std::string to_string(UnaryError, std::string &arg);
  explicit RISCVParserError(NullaryError err, pepp::tc::support::LocationInterval);
  explicit RISCVParserError(UnaryError err, std::string arg1, pepp::tc::support::LocationInterval);
  const pepp::tc::support::LocationInterval loc;
};
} // namespace pepp::tc
