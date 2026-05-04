#pragma once
#include <map>
#include "core/compile/lex/lexer.hpp"
namespace pepp::tc::lex {
struct AsmbOptions {
  bool allow_dot_in_ident = true;
  bool allow_parens = false;          // Tokenize ( and ) as literals rather than invalid tokens.
  bool allow_macro_arguments = false; // Macros arguments start with a backslash like \arg1
  bool allow_at_in_ident = false;     // Allow @ in identifiers, which is used for macros instantiations in Pep/10.
  // Match all GNU as operators, per: https://sourceware.org/binutils/docs-2.24/as/Expressions.html#Expressions
  // Enabling this flag disables parsing of signed integers. Operator precedence is required to determine if +- are
  // unary prefix or binary infix.
  // Multi-character operators need to be "fused" at the parser level.
  bool recognize_operators = false;
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
  // Provide a string view between two locations.
  // Only works for already-parsed locations. If interval outside of parsed text, will return empty stringview.
  std::string_view view(support::LocationInterval loc) const;

private:
  AsmbOptions _opts;
  std::unique_ptr<std::regex> _lineCommentRegex = nullptr;
  // Not needed in base lexer since we only need it to support macros/conditionals.
  std::map<decltype(support::Location::row), size_t> _row_to_streampos;
  std::unordered_set<char> _literals;
};

} // namespace pepp::tc::lex
