#pragma once
#include "../core/common_tokens.hpp"
namespace pepp::tc::lex {

enum class AsmTokenType {
  DotCommand = static_cast<int>(CommonTokenType::_FirstUser) << 0,
  MacroInvocation = static_cast<int>(CommonTokenType::_FirstUser) << 1,
  CharacterConstant = static_cast<int>(CommonTokenType::_FirstUser) << 2,
  StringConstant = static_cast<int>(CommonTokenType::_FirstUser) << 3,
};

struct DotCommand : public Identifier {
  DotCommand(support::LocationInterval loc, support::StringPool *pool, support::PooledString id);
  static constexpr int TYPE = static_cast<int>(AsmTokenType::DotCommand);
  int type() const override;
};

struct MacroInvocation : public Identifier {
  MacroInvocation(support::LocationInterval loc, support::StringPool *pool, support::PooledString id);
  static constexpr int TYPE = static_cast<int>(AsmTokenType::MacroInvocation);
  int type() const override;
};

struct CharacterConstant : public Token {
  CharacterConstant(support::LocationInterval loc, QChar value);
  static constexpr int TYPE = static_cast<int>(AsmTokenType::CharacterConstant);
  int type() const override;
  QString type_name() const override;
  QString to_string() const override;
  QString repr() const override;

  QChar value;
};

// We are going to cheat with string constants. You should drop the quotes when making an ID out of this.
// While the lexer MUST check that escape sequences are valid, it does NOT need to convert them into actual characters.
struct StringConstant : public Identifier {
  StringConstant(support::LocationInterval loc, support::StringPool *pool, support::PooledString id);
  static constexpr int TYPE = static_cast<int>(AsmTokenType::StringConstant);
  int type() const override;
  QString type_name() const override;
  QString to_string() const override;
  QString repr() const override;
};

} // namespace pepp::tc::lex
