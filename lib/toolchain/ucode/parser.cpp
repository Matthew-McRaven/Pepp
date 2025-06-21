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

pepp::ucode::detail::TokenBuffer::TokenBuffer(const QStringView &line) : _data(line) {}

int pepp::ucode::detail::TokenBuffer::matchCount() const { return _matchCount; }

bool pepp::ucode::detail::TokenBuffer::inputRemains() const { return _start < _data.size(); }

bool pepp::ucode::detail::TokenBuffer::match(Token token, QStringView *out) {
  if (peek(token, out)) return _start = _end, _currentToken.reset(), _matchCount++, true;
  else return false;
}

bool pepp::ucode::detail::TokenBuffer::peek(Token token, QStringView *out) {
  static const QRegularExpression identifier("[a-zA-Z][a-zA-Z0-9_]*");
  static const QRegularExpression symbol("[a-zA-Z_][a-zA-Z0-9_]*:");
  static const QRegularExpression decimal("[0-9]+");
  static const QRegularExpression hexadecimal("0[xX][0-9a-fA-F]+");
  static const QRegularExpression comment("//[^\n]*");
  static const auto NormalMatch = QRegularExpression::NormalMatch;
  static const auto Anchored = QRegularExpression::AnchorAtOffsetMatchOption;
  // If we already have a token, check if it matches the requested token.
  if (_currentToken) {
    if (*_currentToken == token) {
      if (out) *out = _data.sliced(_start, _end - _start);
      return true;
    } else return false;
  } else if (!inputRemains()) return false;
  // No current token, so we need to read _data.
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
    } else if (auto maybeSymbol = symbol.matchView(_data, _start, NormalMatch, Anchored); maybeSymbol.hasMatch()) {
      _end = maybeSymbol.capturedEnd(0);
      // UnitPre and UnitPost are constructs that look like symbols. Hijacking symbol code to avoid 2 extra regexs
      if (maybeSymbol.capturedView(0).compare("UnitPre:", Qt::CaseInsensitive) == 0) _currentToken = Token::UnitPre;
      else if (maybeSymbol.capturedView(0).compare("UnitPost:", Qt::CaseInsensitive) == 0)
        _currentToken = Token::UnitPost;
      else _currentToken = Token::Symbol;
      break;
    } else if (auto maybeIdent = identifier.matchView(_data, _start, NormalMatch, Anchored); maybeIdent.hasMatch()) {
      _end = maybeIdent.capturedEnd(0), _currentToken = Token::Identifier;
      break;
    } else if (auto maybeHex = hexadecimal.matchView(_data, _start, NormalMatch, Anchored); maybeHex.hasMatch()) {
      _end = maybeHex.capturedEnd(0), _currentToken = Token::Hexadecimal;
      break;
    } else if (auto maybeDec = decimal.matchView(_data, _start, NormalMatch, Anchored); maybeDec.hasMatch()) {
      _end = maybeDec.capturedEnd(0), _currentToken = Token::Decimal;
      break;
    } else if (auto maybeComment = comment.matchView(_data, _start, NormalMatch, Anchored); maybeComment.hasMatch()) {
      _end = maybeComment.capturedEnd(0), _currentToken = Token::Comment;
      break;
    } else {
      _end = _start + 1, _currentToken = Token::Invalid;
      break;
    }
  }
  return peek(token, out);
}
