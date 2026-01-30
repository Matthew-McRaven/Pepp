#pragma once
#include "core/libs/compile//lex/tokens.hpp"
#include "core/libs/compile/lex/lexer.hpp"
namespace pepp::tc::lex {

enum class MicrocodeTokenType {
  LineNumber = static_cast<int>(CommonTokenType::_FirstUser) << 0,
  UnitPre = static_cast<int>(CommonTokenType::_FirstUser) << 1,
  UnitPost = static_cast<int>(CommonTokenType::_FirstUser) << 2,
};

struct LineNumber : public Token {
  LineNumber(support::LocationInterval loc, int lineNo);
  static constexpr int TYPE = static_cast<int>(MicrocodeTokenType::LineNumber);
  int type() const override;
  std::string type_name() const override;
  std::string to_string() const override;
  std::string repr() const override;

  int line = 0;
};
struct UnitPre : public Token {
  UnitPre(support::LocationInterval loc);
  static constexpr int TYPE = static_cast<int>(MicrocodeTokenType::UnitPre);
  int type() const override;
  std::string type_name() const override;
  std::string to_string() const override;
  std::string repr() const override;
};
struct UnitPost : public Token {
  UnitPost(support::LocationInterval loc);
  static constexpr int TYPE = static_cast<int>(MicrocodeTokenType::UnitPost);
  int type() const override;
  std::string type_name() const override;
  std::string to_string() const override;
  std::string repr() const override;
};
} // namespace pepp::tc::lex
