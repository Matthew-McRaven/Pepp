#pragma once
#include "core/compile/lex/lexer.hpp"
#include "core/compile/lex/tokens.hpp"

namespace pepp::tc::lex {

struct MicroLexer : public ALexer {
  MicroLexer(std::shared_ptr<std::unordered_set<std::string>> identifier_pool, support::SeekableData &&data);
  ~MicroLexer() override = default;
  bool input_remains() const override;
  std::shared_ptr<Token> next_token() override;
};
} // namespace pepp::tc::lex
