#pragma once
#include <stack>
#include "../core/lexer.hpp"
namespace pepp::tc::lex {

struct PepLexer : public ALexer {
  PepLexer(std::shared_ptr<support::StringPool> identifier_pool, support::SeekableData &&data);
  ~PepLexer() override = default;
  bool input_remains() const override;
  std::shared_ptr<Token> next_token() override;
};

// This lexer does an evil trick.
// When it hits a macro invocation, it will switch into the lexer for the macro body before silently returning to the
// parent. This way macro parsing can be entirely transparent to the rest of the assembler.
// It has to do some trickery when it encounters a macro invocation token, because it has to parse the arguments and
// perform textual substitution before lexing the macro body.
struct ChainMacroLexer : public ALexer {
  ChainMacroLexer(std::shared_ptr<support::StringPool> identifier_pool, support::SeekableData &&data,
                  std::shared_ptr<void *> macro_registry);
  ~ChainMacroLexer() override = default;
  bool input_remains() const override;
  std::shared_ptr<Token> next_token() override;

private:
  std::stack<PepLexer> _lexer_chain;
};
} // namespace pepp::tc::lex
