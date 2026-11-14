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
#include "core/langs/expr/lexer.hpp"
#include <regex>
#include "core/compile/lex/tokens.hpp"
#include "core/math/bitmanip/strings.hpp"

namespace {
using Identifiers = pepp::tc::expr::Options::Identifiers;
using Integers = pepp::tc::expr::Options::Integers;

// Each takes an optional % for function names such as %hi. Lexer should manually compare leading character against
// percent_identifiers.
const std::regex &identifier_pattern(Identifiers identifiers) {
  static const std::regex alphabetic("%?[a-zA-Z_][a-zA-Z0-9_]*");
  static const std::regex leading_dot(R"(%?\.?[a-zA-Z_][a-zA-Z0-9_]*|\.)");
  static const std::regex dotted(R"(%?[a-zA-Z_.$][a-zA-Z0-9_.$]*)");
  switch (identifiers) {
  case Identifiers::Alphabetic: return alphabetic;
  case Identifiers::LeadingDot: return leading_dot;
  case Identifiers::Dotted: return dotted;
  }
  return alphabetic;
}

// Text starting with a digit that is lexed as one number.
const std::regex &number_pattern(Integers integers) {
  // Includes a bare 0x so that it is invalid rather than 0 followed by x.
  static const std::regex c("0[xX][0-9a-fA-F]*|[0-9]+");
  // Match all characters after the leading position to account for various suffixes / multipliers.
  static const std::regex ld(R"([0-9][a-zA-Z0-9_.$]*)");
  switch (integers) {
  case Integers::C: return c;
  case Integers::Ld: return ld;
  }
  return c;
}

std::optional<bits::ParsedInteger> parse_integer(std::string_view word, Integers integers) {
  switch (integers) {
  case Integers::C: return bits::c_integer(word);
  case Integers::Ld: return bits::ld_integer(word);
  }
  return std::nullopt;
}
} // namespace

pepp::tc::expr::ExpressionLexer::ExpressionLexer(std::shared_ptr<std::unordered_set<std::string>> identifier_pool,
                                                 support::SeekableData &&data, Options options)
    : ALexer(std::move(identifier_pool), std::move(data)), _options(options) {}

bool pepp::tc::expr::ExpressionLexer::input_remains() const { return _cursor.input_remains(); }

std::shared_ptr<pepp::tc::lex::Token> pepp::tc::expr::ExpressionLexer::next_token() {
  using namespace pepp::tc::lex;
  using Integer = pepp::tc::lex::Integer;
  using LocationInterval = support::LocationInterval;
  const auto &identifier = identifier_pattern(_options.identifiers);
  static const std::regex charConstant(R"('([^'\\]|\\[bfvnrt\\0'"]|\\[xX][0-9a-fA-F]{2})')");
  // Two-character operators first, so that << is not lexed as < <.
  // A comma inside a function definition is used as a separator and otherwise should end expressions.
  static const std::regex op(R"(<<|>>|<=|>=|==|!=|&&|\|\||[-+*/%<>&^|~!(),])");

  while (input_remains()) { // Consume leading whitespace.
    const auto next = _cursor.peek();
    if (next == ' ' || next == '\t' || next == '\r') _cursor.skip(1);
    else if (_options.newline_whitespace && next == '\n') _cursor.skip(1), _cursor.newline();
    else if (!_options.block_comments || !skip_block_comment()) break;
  }

  const auto start = _cursor.location();
  std::shared_ptr<Token> current_token;
  if (!input_remains()) current_token = std::make_shared<EoF>(LocationInterval{start, start});
  else if (_cursor.peek() == '\n') {
    _cursor.advance(1);
    _cursor.newline();
    current_token = std::make_shared<Empty>(since(start));
  } else if (_options.block_comments &&
             _cursor.rest().starts_with("/*")) { // Unclosed comment; whitespace skips the rest.
    _cursor.advance(_cursor.rest().size());
    current_token = invalid(start);
  } else if (match_advance(number_pattern(_options.integers))) {
    current_token = integer(start, parse_integer(_cursor.select(), _options.integers));
  } else if (_options.character_constants && match_advance(charConstant)) {
    const auto text = _cursor.select();
    // Omit open and close quotes.
    current_token = std::make_shared<CharacterConstant>(since(start), std::string{text.substr(1, text.size() - 2)});
  } else if ((_options.percent_identifiers || _cursor.peek() != '%') && match_advance(identifier)) {
    // ld also has numbers that start with a letter or $, such as ax and $1F.
    const auto parsed = _options.integers == Integers::Ld ? bits::ld_integer(_cursor.select()) : std::nullopt;
    current_token = parsed ? integer(start, parsed) : name(start);
  } else if (match_advance(op)) {
    current_token = std::make_shared<Literal>(since(start), std::string{_cursor.select()});
  } else {
    _cursor.advance(1);
    current_token = invalid(start);
  }
  _cursor.skip(0);
  return current_token;
}

std::shared_ptr<pepp::tc::lex::Token> pepp::tc::expr::ExpressionLexer::invalid(support::Location start) const {
  return std::make_shared<lex::Invalid>(since(start), std::string{_cursor.select()});
}

std::shared_ptr<pepp::tc::lex::Token>
pepp::tc::expr::ExpressionLexer::integer(support::Location start,
                                         const std::optional<bits::ParsedInteger> &parsed) const {
  if (!parsed) return invalid(start);
  const auto format = parsed->base == 16 ? lex::Integer::Format::Hex : lex::Integer::Format::UnsignedDec;
  return std::make_shared<lex::Integer>(since(start), parsed->value, format);
}

std::shared_ptr<pepp::tc::lex::Token> pepp::tc::expr::ExpressionLexer::name(support::Location start) const {
  const auto location = since(start);
  const auto text = _cursor.select();
  if (text != ".") return std::make_shared<lex::Identifier>(location, &*_pool->emplace(text).first);
  return std::make_shared<lex::Literal>(location, ".");
}
