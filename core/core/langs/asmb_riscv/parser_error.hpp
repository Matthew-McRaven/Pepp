#pragma once

#include <stdexcept>
#include "core/compile/source/location.hpp"
namespace pepp::tc {
class ParserError final : public std::logic_error {
public:
  enum class NullaryError {
    Argument_ExpectedRD,
    Argument_ExpectedRS1,
    Argument_ExpectedRS2,
    Token_MissingNewline,
    Token_MissingComma,
  };
  enum class UnaryError { Token_Invalid };

  static const std::string to_string(NullaryError);
  static const std::string to_string(UnaryError, std::string &arg);
  explicit ParserError(NullaryError err, pepp::tc::support::LocationInterval);
  explicit ParserError(UnaryError err, std::string arg1, pepp::tc::support::LocationInterval);
  const pepp::tc::support::LocationInterval loc;
};
} // namespace pepp::tc
