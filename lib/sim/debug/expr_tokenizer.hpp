#pragma once
#include <QtCore>
#include "sim/debug/expr_value.hpp"

namespace pepp::debug {

namespace detail {
enum class TokenType {
  UnsignedConstant,
  TypeSuffix,
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

template <> struct T<TokenType::TypeSuffix> {
  pepp::debug::types::Primitives type;
  std::strong_ordering operator<=>(const T<TokenType::TypeSuffix> &rhs) const;
};
using TypeSuffix = T<TokenType::TypeSuffix>;

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
    std::variant<std::monostate, Invalid, Eof, UnsignedConstant, TypeSuffix, Identifier, DebugIdentifier, Literal>;
} // namespace detail

class Lexer {
public:
  explicit Lexer(QStringView input);
  using Token = detail::Token;
  Token next_token();
  static std::optional<types::Primitives> primitive_from_string(const QString &str);

private:
  QStringView _input;
  int _offset = 0;
  bool _allows_literal_suffix = false;
};

} // namespace pepp::debug
