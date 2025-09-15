#pragma once
#include "../core/common_tokens.hpp"
#include "../core/lexer.hpp"

namespace pepp::tc::lex {

struct MicroLexer : public ALexer {
  MicroLexer(std::shared_ptr<support::StringPool> identifier_pool, support::SeekableData &&data);
  ~MicroLexer() override = default;
  bool input_remains() const override;
  std::shared_ptr<Token> next_token() override;
};
} // namespace pepp::tc::lex
