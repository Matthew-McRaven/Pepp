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
#include <charconv>
#include <regex>
#include "core/compile/lex/tokens.hpp"
#include "core/langs/asmb/asmb_tokens.hpp"

pepp::tc::expr::ExpressionLexer::ExpressionLexer(std::shared_ptr<std::unordered_set<std::string>> identifier_pool,
                                                 support::SeekableData &&data)
    : ALexer(std::move(identifier_pool), std::move(data)) {}

bool pepp::tc::expr::ExpressionLexer::input_remains() const { return _cursor.input_remains(); }

std::shared_ptr<pepp::tc::lex::Token> pepp::tc::expr::ExpressionLexer::next_token() {
  using namespace pepp::tc::lex;
  using Integer = pepp::tc::lex::Integer;
  using LocationInterval = support::LocationInterval;
  static const std::regex identifier("[a-zA-Z_][a-zA-Z0-9_]*");
  static const std::regex decimal("[0-9]+");
  static const std::regex hexadecimal("0[xX][0-9a-fA-F]+");
  static const std::regex badHex("0[xX]");
  static const std::regex charConstant(R"('([^'\\]|\\[bvnrt\\0']|\\[xX][0-9a-fA-F]{2})')");
  // Two-character operators first, so that << is not lexed as < <.
  static const std::regex op(R"(<<|>>|<=|>=|==|!=|&&|\|\||[-+*/%<>&^|~!()])");

  while (input_remains()) { // Consume leading whitespace.
    const auto next = _cursor.peek();
    if (next != ' ' && next != '\t' && next != '\r') break;
    _cursor.skip(1);
  }

  const auto loc_start = _cursor.location();
  const auto here = [&] { return LocationInterval{loc_start, _cursor.location()}; };
  const auto invalid = [&]() -> std::shared_ptr<Token> {
    return std::make_shared<Invalid>(here(), std::string{_cursor.select()});
  };

  // Integer literals must fit in 64 bits as either signed or unsigned bit patterns.
  const auto integer = [&](std::string_view digits, int base, Integer::Format format) -> std::shared_ptr<Token> {
    u64 value = 0;
    // The regex has already checked the digits, so only overflow can fail.
    if (std::from_chars(digits.data(), digits.data() + digits.size(), value, base).ec != std::errc{}) return invalid();
    return std::make_shared<Integer>(here(), value, format);
  };

  // On a match, advance over it. _cursor.select() is then whole the token's text.
  std::shared_ptr<Token> current_token;
  if (!input_remains()) current_token = std::make_shared<EoF>(here());
  else if (_cursor.peek() == '\n') {
    _cursor.advance(1);
    _cursor.newline();
    current_token = std::make_shared<Empty>(here());
  } else if (auto maybeHex = _cursor.matchView(hexadecimal); !maybeHex.empty()) {
    _cursor.advance(maybeHex.length(0));
    current_token = integer(_cursor.select().substr(2), 16, Integer::Format::Hex);
  } else if (auto maybeBadHex = _cursor.matchView(badHex); !maybeBadHex.empty()) {
    _cursor.advance(maybeBadHex.length(0));
    current_token = invalid();
  } else if (auto maybeDec = _cursor.matchView(decimal); !maybeDec.empty()) {
    _cursor.advance(maybeDec.length(0));
    current_token = integer(_cursor.select(), 10, Integer::Format::UnsignedDec);
  } else if (auto maybeChar = _cursor.matchView(charConstant); !maybeChar.empty()) {
    _cursor.advance(maybeChar.length(0));
    const auto text = _cursor.select();
    // Omit open and close quotes.
    current_token = std::make_shared<CharacterConstant>(here(), std::string{text.substr(1, text.size() - 2)});
  } else if (auto maybeIdent = _cursor.matchView(identifier); !maybeIdent.empty()) {
    _cursor.advance(maybeIdent.length(0));
    auto const *id = &*_pool->emplace(_cursor.select()).first;
    current_token = std::make_shared<Identifier>(here(), id);
  } else if (auto maybeOp = _cursor.matchView(op); !maybeOp.empty()) {
    _cursor.advance(maybeOp.length(0));
    current_token = std::make_shared<Literal>(here(), std::string{_cursor.select()});
  } else {
    _cursor.advance(1);
    current_token = invalid();
  }
  _cursor.skip(0);
  return current_token;
}
