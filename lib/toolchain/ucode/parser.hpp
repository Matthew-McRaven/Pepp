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
#pragma once
#include <QRegularExpression>
#include "utils/bits/log2.hpp"
namespace pepp::ucode {
template <typename uarch> struct ParseResult {
  using Error = std::pair<int, QString>;
  using Errors = std::vector<Error>;
  struct Line {
    typename uarch::CodeWithEnables controls;
    std::optional<QString> comment, symbolDecl;
    uint16_t address = -1;
  };

  using Program = std::vector<Line>;
  Errors errors;
  QMap<QString, std::optional<uint16_t>> symbols;
  Program program;
};
template <typename uarch> std::optional<std::string> format(const std::string &source) { return std::nullopt; }
namespace detail {
template <typename uarch>
bool parseLine(const QStringView &line, typename ParseResult<uarch>::Line &code, QString &error);
} // namespace detail
template <typename uarch> ParseResult<uarch> parse(const QString &source) {
  ParseResult<uarch> result;
  int startIdx = 0, endIdx = 0, lineNumber = 0, addressCounter = 0;
  while ((endIdx = source.indexOf('\n', startIdx)) != -1) {
    auto line = QStringView(source).mid(startIdx, endIdx - startIdx);
    typename ParseResult<uarch>::Line codeLine;
    QString error;
    if (detail::parseLine<uarch>(line, codeLine, error)) {
      // Handle address assignment, assigning symbol values.
      if (codeLine.controls.enables.any()) codeLine.address = addressCounter++;
      if (codeLine.symbolDecl.has_value()) {
        auto symbol = *codeLine.symbolDecl;
        if (result.symbols.contains(symbol))
          result.errors.emplace_back(std::make_pair(lineNumber, "Symbol already defined: " + symbol));
        else result.symbols[symbol] = codeLine.address;
      }
      result.program.emplace_back(codeLine);
    } else result.errors.emplace_back(std::make_pair(lineNumber, error));
    startIdx = endIdx + 1;
    lineNumber++;
  }
  return result;
}

namespace detail {
enum class Token {
  Empty = 0,
  Comma,
  Equals,
  Semicolon,
  LeftBracket,
  RightBracket,
  Identifier,
  Comment,
  Decimal,
  Hexadecimal,
  Symbol,
  Invalid
};
class TokenBuffer {
public:
  TokenBuffer(const QStringView &line);
  bool inputRemains() const;
  bool match(Token token, QStringView *out = nullptr);
  bool peek(Token token, QStringView *out = nullptr);
  QStringView rest();

private:
  static const QRegularExpression _identifier;
  static const QRegularExpression _symbol;
  static const QRegularExpression _decimal;
  static const QRegularExpression _hexadecimal;
  static const QRegularExpression _comment;

  const QStringView &_data;
  int _start = 0, _end = 0;
  std::optional<Token> _currentToken = std::nullopt;
  ;
};

} // namespace detail

template <typename uarch>
bool detail::parseLine(const QStringView &line, typename ParseResult<uarch>::Line &code, QString &error) {
  int current_group = 0;
  TokenBuffer buf(line);
  QStringView current;
  while (buf.inputRemains()) {
    if (buf.match(Token::Symbol, &current)) {
      if (!uarch::allows_symbols()) return error = "Symbols are forbidden", false;
      // Remove trailing :
      code.symbolDecl = current.left(current.size() - 1).toString();
      if (!buf.peek(Token::Identifier)) return error = "Expected identifier after symbol declaration", false;
    } else if (buf.match(Token::Semicolon)) {
      current_group++;
      if (current_group > uarch::max_signal_groups()) return error = "Unexpected semicolon", false;
    } else if (buf.match(Token::Identifier, &current)) {
      auto maybe_signal = uarch::parse_signal(current);
      if (!maybe_signal.has_value()) return error = "Unknown signal: " + current, false;
      typename uarch::Signals s = *maybe_signal;

      if (uarch::is_clock(s)) {
        if (code.controls.enabled(s)) return error = "Clock signal already defined", false;
        else if (uarch::signal_group(s) != current_group) return error = "Unexpected signal " + current, false;
        else code.controls.set(s, 1);

        if (buf.match(Token::Comma) || buf.match(Token::Empty)) continue;
        else if (buf.match(Token::Comment, &current)) code.comment = current.toString();
        else if (buf.peek(Token::Semicolon) || !buf.inputRemains()) continue;
        else return (error = "Unexpected token after clock signal"), false;
      } else {
        if (uarch::signal_group(s) != current_group) return error = "Unexpected signal " + current, false;
        else if (!buf.match(Token::Equals)) return error = "Expected '=' after signal", false;
        else if (!buf.match(Token::Decimal, &current)) return error = "Expected decimal value for signal", false;

        bool ok = false;
        if (int value = current.toInt(&ok); !ok) return error = "Failed to parse signal value", false;
        else if (code.controls.enabled(s)) return error = "Signal already defined", false;
        else if (uarch::signal_bit_size(s) < (value == 0 ? 1 : bits::ceil_log2(value)))
          return error = "Signal value too large", false;
        else code.controls.set(s, value);

        if (buf.match(Token::Comma) || buf.match(Token::Empty)) continue;
        else if (buf.match(Token::Comment, &current)) code.comment = current.toString();
        else if (buf.peek(Token::Semicolon) || !buf.inputRemains()) continue;
        else return error = "Unexpected token after signal", false;
      }
    } else if (buf.match(Token::Comment, &current)) return code.comment = current.toString(), true;
    else if (buf.match(Token::Empty)) return true;
    else return error = "Unexpected token: " + buf.rest(), false;
  }
  return true;
}
} // namespace pepp::ucode
