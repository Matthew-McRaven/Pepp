#pragma once
#include <QtCore>
#include "sim/debug/expr_types.hpp"

namespace pepp::debug {

namespace detail {
enum class TokenType {
  UnsignedConstant,
  ConstantType,
  DebugIdentifier,
  Identifier,
  Literal,
  Invalid,
  Eof,
};

template <TokenType type> struct T {};

template <> struct T<TokenType::Invalid> {};
using Invalid = T<TokenType::Invalid>;

template <> struct T<TokenType::Eof> {};
using Eof = T<TokenType::Eof>;

template <> struct T<TokenType::UnsignedConstant> {
  enum class Format { Dec, Hex };
  Format format;
  uint64_t value = 0xFEED;

  std::strong_ordering operator<=>(const T<TokenType::UnsignedConstant> &rhs) const;
};
using UnsignedConstant = T<TokenType::UnsignedConstant>;

template <> struct T<TokenType::ConstantType> {
  pepp::debug::ExpressionType type;
  std::strong_ordering operator<=>(const T<TokenType::ConstantType> &rhs) const;
};
using ConstantType = T<TokenType::ConstantType>;

template <> struct T<TokenType::Identifier> {
  QString value;
};
using Identifier = T<TokenType::Identifier>;

template <> struct T<TokenType::DebugIdentifier> {
  QString value;
};
using DebugIdentifier = T<TokenType::DebugIdentifier>;

template <> struct T<TokenType::Literal> {
  QString literal;
};
using Literal = T<TokenType::Literal>;

using Token =
    std::variant<std::monostate, Invalid, Eof, UnsignedConstant, ConstantType, Identifier, DebugIdentifier, Literal>;
} // namespace detail

class Lexer {
public:
  explicit Lexer(QStringView input);
  using Token = detail::Token;
  Token next_token();

private:
  QStringView _input;
  int _offset = 0;
  bool _allows_trailing_type = false;
};

} // namespace pepp::debug
