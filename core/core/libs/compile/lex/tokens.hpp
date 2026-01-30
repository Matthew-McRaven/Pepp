#pragma once
#include <string>
#include "core/libs/compile/source/location.hpp"

namespace pepp::tc::lex {
enum class CommonTokenType {
  Invalid = 1 << 0,
  EoF = 1 << 1,
  Empty = 1 << 2,
  Integer = 1 << 3,
  Identifier = 1 << 4,
  InlineComment = 1 << 5,
  SymbolDeclaration = 1 << 6,
  Literal = 1 << 7,
  _FirstUser = 1 << 8,
};

struct Token {
  // Should default construct the invalid interval
  explicit Token(tc::support::LocationInterval loc = {});
  virtual ~Token() = 0;
  virtual int type() const = 0;
  virtual std::string type_name() const = 0;
  // Several tokens are unrepresentable (e.g., EoF/Invalid), so default to empty string.
  // Cannot use QStringView since some classes (e.g., Integer) do not have a backing QString.
  // This should be the string representing the value contained in the token.
  virtual std::string to_string() const;
  // A more detailed representation of the token, including both the name and the value. Useful for debugging
  virtual std::string repr() const;
  bool mask(int mask) const;

  support::LocationInterval location() const;
  support::Location start() const;
  support::Location end() const;

protected:
  support::LocationInterval _loc;
};

// Common token types across all lexers.
struct Invalid : public Token {
  // You should be able to provide a loc for an invalid token!
  explicit Invalid(support::LocationInterval loc);
  static constexpr int TYPE = static_cast<int>(CommonTokenType::Invalid);
  int type() const override;
  std::string type_name() const override;
};

struct EoF : public Token {
  explicit EoF(support::LocationInterval loc);
  static constexpr int TYPE = static_cast<int>(CommonTokenType::EoF);
  int type() const override;
  std::string type_name() const override;
};

struct Empty : public Token {
  explicit Empty(support::LocationInterval loc);
  static constexpr int TYPE = static_cast<int>(CommonTokenType::Empty);
  int type() const override;
  std::string type_name() const override;
  std::string to_string() const override;
  std::string repr() const override;
};

struct Integer : public Token {
  enum class Format { SignedDec, UnsignedDec, Hex, Bin } format = Format::SignedDec;
  explicit Integer(support::LocationInterval loc, uint64_t val = 0, Format fmt = Format::SignedDec);
  static constexpr int TYPE = static_cast<int>(CommonTokenType::Integer);
  int type() const override;
  std::string type_name() const override;
  std::string to_string() const override;

  uint64_t value = 0;
};

struct Identifier : public Token {
  Identifier(support::LocationInterval loc, std::string const *v);
  static constexpr int TYPE = static_cast<int>(CommonTokenType::Identifier);
  int type() const override;
  std::string_view view() const;
  std::string type_name() const override;
  std::string to_string() const override;
  std::string const *value;
};

struct SymbolDeclaration : public Identifier {
  SymbolDeclaration(support::LocationInterval loc, std::string const *v);
  static constexpr int TYPE = static_cast<int>(CommonTokenType::SymbolDeclaration);
  int type() const override;
  std::string type_name() const override;
};

// For comments of the form ";hi", or "// hi" which do not span more than one line
struct InlineComment : public Identifier {
  InlineComment(support::LocationInterval loc, std::string const *v);
  static constexpr int TYPE = static_cast<int>(CommonTokenType::InlineComment);
  int type() const override;
  std::string type_name() const override;
};

// Used for single / very short character literals, like <|=>
struct Literal : public Token {
  Literal(support::LocationInterval loc, std::string literal);
  static constexpr int TYPE = static_cast<int>(CommonTokenType::Literal);
  int type() const override;
  std::string type_name() const override;
  std::string to_string() const override;

  std::string literal;
};

} // namespace pepp::tc::lex
