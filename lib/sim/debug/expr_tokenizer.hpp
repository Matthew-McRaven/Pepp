#pragma once
#include <QtCore>

namespace pepp::debug {

namespace detail {
enum class TokenType {
  UnsignedConstant,
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
  enum class Format { Dec, Hex } format;
  uint64_t value = 0xFEED;
};
using UnsignedConstant = T<TokenType::UnsignedConstant>;

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

using Token = std::variant<std::monostate, Invalid, Eof, UnsignedConstant, Identifier, DebugIdentifier, Literal>;
} // namespace detail

class Lexer {
public:
  Lexer(QStringView input);
  using Token = detail::Token;
  Token next_token();

private:
  QStringView _input;
  int _offset = 0;
};

} // namespace pepp::debug
