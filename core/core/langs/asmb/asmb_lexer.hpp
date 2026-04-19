#pragma once
#include "core/compile/lex/lexer.hpp"
namespace pepp::tc::lex {
struct AsmbOptions {
  bool allow_dot_in_ident = true;
  bool allow_parens = false;          // Tokenize ( and ) as literals rather than invalid tokens.
  bool allow_macro_arguments = false; // Macros arguments start with a backslash like \arg1
  bool allow_at_in_ident = false;     // Allow @ in identifiers, which is used for macros instatniations in Pep/10.
  std::string line_comment_leader = ";";
};
struct AsmbLexer : public ALexer {

  AsmbLexer(std::shared_ptr<std::unordered_set<std::string>> identifier_pool, support::SeekableData &&data,
            AsmbOptions options = AsmbOptions{});
  ~AsmbLexer() override = default;
  AsmbLexer(const AsmbLexer &) = delete;
  AsmbLexer &operator=(const AsmbLexer &) = delete;
  AsmbLexer(AsmbLexer &&) noexcept = default;
  AsmbLexer &operator=(AsmbLexer &&) = default;

  bool input_remains() const override;
  std::shared_ptr<Token> next_token() override;

private:
  AsmbOptions _opts;
  std::unique_ptr<std::regex> _lineCommentRegex = nullptr;
};

} // namespace pepp::tc::lex
