/*
 * Copyright (c) 2023-2025 J. Stanley Warford, Matthew McRaven
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
#include "./parser.hpp"

const QRegularExpression pepp::ucode::detail::TokenBuffer::_identifier("[a-zA-Z]+");
const QRegularExpression pepp::ucode::detail::TokenBuffer::_decimal("[0-9]+");
const QRegularExpression pepp::ucode::detail::TokenBuffer::_hexadecimal("0[xX][0-9a-fA-F]+");
const QRegularExpression pepp::ucode::detail::TokenBuffer::_comment(";[^\n]*");

pepp::ucode::detail::TokenBuffer::TokenBuffer(const QStringView &line) : _data(line) {}

bool pepp::ucode::detail::TokenBuffer::inputRemains() const { return _start < _data.size(); }

bool pepp::ucode::detail::TokenBuffer::match(Token token, QStringView *out) {
  if (peek(token, out)) return _start = _end, _currentToken.reset(), true;
  else return false;
}

bool pepp::ucode::detail::TokenBuffer::peek(Token token, QStringView *out) {
  static const auto NormalMatch = QRegularExpression::NormalMatch;
  static const auto Anchored = QRegularExpression::AnchorAtOffsetMatchOption;
  if (_currentToken) {
    if (*_currentToken == token) {
      if (out) *out = _data.sliced(_start, _end - _start);
      return true;
    } else return false;
  } else if (!inputRemains()) return false;
  while (inputRemains()) {
    auto nextCh = _data[_start];
    if (nextCh == "\n") {
      _end = _start + 1, _currentToken = Token::Empty;
      break;
    } else if (nextCh.isSpace()) {
      _start++;
      continue;
    } else if (nextCh == "[") {
      _end = _start + 1, _currentToken = Token::LeftBracket;
      break;
    } else if (nextCh == "]") {
      _end = _start + 1, _currentToken = Token::RightBracket;
      break;
    } else if (nextCh == "=") {
      _end = _start + 1, _currentToken = Token::Equals;
      break;
    } else if (nextCh == ",") {
      _end = _start + 1, _currentToken = Token::Comma;
      break;
    } else if (nextCh == ";") {
      _end = _start + 1, _currentToken = Token::Semicolon;
      break;
    } else if (auto maybeIdent = _identifier.matchView(_data, _start, NormalMatch, Anchored); maybeIdent.hasMatch()) {
      _end = maybeIdent.capturedEnd(0), _currentToken = Token::Identifier;
      break;
    } else if (auto maybeDec = _decimal.matchView(_data, _start, NormalMatch, Anchored); maybeDec.hasMatch()) {
      _end = maybeDec.capturedEnd(0), _currentToken = Token::Decimal;
      break;
    } else if (auto maybeHex = _hexadecimal.matchView(_data, _start, NormalMatch, Anchored); maybeHex.hasMatch()) {
      _end = maybeHex.capturedEnd(0), _currentToken = Token::Hexadecimal;
      break;
    } else if (auto maybeComment = _comment.matchView(_data, _start, NormalMatch, Anchored); maybeComment.hasMatch()) {
      _end = maybeComment.capturedEnd(0), _currentToken = Token::Comment;
      break;
    } else {
      _end = _start + 1, _currentToken = Token::Invalid;
      break;
    }
  }
  return peek(token, out);
}

QStringView pepp::ucode::detail::TokenBuffer::rest() { return _data.mid(_end); }
