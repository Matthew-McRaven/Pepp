/*
 * Copyright (c) 2026. Stanley Warford, Matthew McRaven
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "core/langs/ldscript/lexer.hpp"
#include <regex>
#include "core/compile/lex/tokens.hpp"

namespace {
// Character set for identifiers. Excludes = and : so that x=1 and .text: lex without spaces.
const std::regex &identifier_pattern() {
  static const std::regex name(R"([A-Za-z0-9_.$/\\~+\-\[\]*?!^]+)");
  return name;
}
} // namespace

bool pepp::tc::ldscript::is_unquoted_identifier(std::string_view text) {
  return std::regex_match(text.begin(), text.end(), identifier_pattern());
}

pepp::tc::ldscript::Lexer::Lexer(std::shared_ptr<std::unordered_set<std::string>> identifier_pool,
                                 support::SeekableData &&data)
    : ALexer(std::move(identifier_pool), std::move(data)) {}

bool pepp::tc::ldscript::Lexer::input_remains() const { return _cursor.input_remains(); }

// Since constructs span multiple lines, \r and \n are treated as whitespace.
void pepp::tc::ldscript::Lexer::skip_whitespace() {
  _cursor.skip(0);
  while (_cursor.input_remains()) {
    const auto next = _cursor.peek();
    if (next == '\n') _cursor.skip(1), _cursor.newline();
    else if (next == ' ' || next == '\t' || next == '\r') _cursor.skip(1);
    else break;
  }
}

std::shared_ptr<pepp::tc::lex::Token> pepp::tc::ldscript::Lexer::next_token() {
  using namespace pepp::tc::lex;
  // Must be matched before names, since * and - are in the identifier character set.
  static const std::regex assignment(R"(<<=|>>=|[-+*/&|]?=)");
  static const std::regex separator(R"([{}();:,])");
  static const std::regex string(R"("[^"\n]*")");
  static const std::regex line_comment("#[^\n]*");
  // Also matches an unclosed comment, which consumes the rest of the input.
  static const std::regex block_comment(R"(/\*[\s\S]*?(\*/|$))");
  skip_whitespace();
  const auto start = _cursor.location();
  const auto pooled = [&](std::string_view text) { return &*_pool->emplace(text).first; };

  std::shared_ptr<Token> ret;
  if (!_cursor.input_remains()) ret = std::make_shared<EoF>(since(start));
  else if (match_advance(line_comment))
    ret = std::make_shared<InlineComment>(since(start), pooled(_cursor.select().substr(1))); // Drop leading #
  else if (const auto match = _cursor.matchView(block_comment); !match.empty()) {
    // Advance one character at a time, so that newlines in the comment update the location.
    for (auto it = match[0].first; it != match[0].second; ++it) {
      _cursor.advance(1);
      if (*it == '\n') _cursor.newline();
    }
    const auto text = _cursor.select();
    if (text.size() < 4 || !text.ends_with("*/")) ret = std::make_shared<Invalid>(since(start), std::string(text));
    else ret = std::make_shared<BlockComment>(since(start), pooled(text.substr(2, text.size() - 4))); // Drop /* */
  } else if (match_advance(assignment) || match_advance(separator))
    ret = std::make_shared<Literal>(since(start), std::string(_cursor.select()));
  else if (match_advance(string)) {
    const auto text = _cursor.select();
    ret = std::make_shared<StringConstant>(since(start), pooled(text.substr(1, text.size() - 2)));
  } else if (match_advance(identifier_pattern()))
    ret = std::make_shared<Identifier>(since(start), pooled(_cursor.select()));
  else {
    // An unclosed string consumes the rest of the input.
    _cursor.advance(_cursor.rest().starts_with('"') ? _cursor.rest().size() : 1);
    ret = std::make_shared<Invalid>(since(start), std::string(_cursor.select()));
  }
  // matchView starts at the selection, so move the selection past this token before expr::parse reads from _cursor.
  _cursor.skip(0);
  return ret;
}
