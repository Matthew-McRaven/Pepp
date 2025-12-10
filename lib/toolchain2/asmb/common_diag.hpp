#pragma once

#include <map>
#include <stdexcept>
#include "toolchain2/support/source/location.hpp"
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
    AddressingMode_Required,
    AddressingMode_Invalid,
    AddressingMode_Missing,
    SymbolDeclaration_Required,
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

class DiagnosticTable {
public:
  void add_message(pepp::tc::support::LocationInterval, std::string);
  auto cbegin() const { return _raw.cbegin(); }
  auto cend() const { return _raw.cend(); }
  auto overlapping_interval(support::LocationInterval i) const {
    auto lb = i.lower().valid() ? _raw.lower_bound(i.lower()) : _raw.cbegin();
    auto ub = i.upper().valid() ? _raw.upper_bound(i.upper()) : _raw.cend();
    return std::pair{lb, ub};
  }
  size_t count() const;

private:
  std::multimap<pepp::tc::support::LocationInterval, std::string> _raw;
};
} // namespace pepp::tc
