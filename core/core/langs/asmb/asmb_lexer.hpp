/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include <map>
#include "core/compile/lex/lexer.hpp"
namespace pepp::tc::lex {
struct AsmbOptions {
  bool allow_dot_in_ident = true;
  bool allow_parens = false;          // Tokenize ( and ) as literals rather than invalid tokens.
  bool allow_macro_arguments = false; // Macros arguments start with a backslash like \arg1
  bool allow_at_in_ident = false;     // Allow @ in identifiers, which is used for macros instantiations in Pep/10.
  bool allow_local_labels = false;    // Declare a GNU local label, .L<name>:, rather than lexing a dot command.
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
