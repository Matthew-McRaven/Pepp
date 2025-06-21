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
namespace pepp::ucode {
struct MemValue {
  quint8 value = 0;
  quint16 address = 0;
};
template <typename registers> struct RegisterValue {
  typename registers::NamedRegisters reg;
  // Based on the size of the register, you MUST mask this down to [1-3] bytes
  quint32 value = 0;
};
template <typename registers> using Test = std::variant<MemValue, RegisterValue<registers>>;

template <typename uarch, typename registers> struct ParseResult {
  using Error = std::pair<int, QString>;
  using Errors = std::vector<Error>;
  struct Line {
    typename uarch::CodeWithEnables controls;
    std::optional<QString> comment, symbolDecl;
    uint16_t address = -1;
    QMap<typename uarch::Signals, QString> deferredValues;
    enum class Type { Code, Pre, Post } type = Type::Code;
    QList<Test<registers>> tests;
  };

  using Program = std::vector<Line>;
  Errors errors;
  QMap<QString, std::optional<uint16_t>> symbols;
  Program program;
};
template <typename uarch, typename registers> QString format(const typename ParseResult<uarch, registers>::Line &line) {
  QString symbolDecl;
  if (line.symbolDecl.has_value()) symbolDecl = *line.symbolDecl + ": ";
  auto _signals = line.controls.toString();
  return QString("%1%2%3").arg(symbolDecl, _signals, line.comment.has_value() ? *line.comment : "");
}
namespace detail {
template <typename uarch, typename registers>
bool parseLine(const QStringView &line, typename ParseResult<uarch, registers>::Line &code, QString &error);
} // namespace detail

// The vector of lines produced by parse is hard to execute, since there may be gaps between executable lines.
// This method extract only lines which contain control signals, ignoring comment-only lines, test lines, etc.
// The result is usable by a microcode simulator without any further post-processing.
template <typename uarch, typename registers>
std::vector<typename uarch::Code> microcodeFor(const ParseResult<uarch, registers> &result) {
  std::vector<typename uarch::Code> ret;
  for (const auto &line : result.program)
    if (line.type == ParseResult<uarch, registers>::Line::Type::Code && line.controls.enables.any())
      ret.emplace_back(line.controls.code);
  return ret;
}

template <typename uarch, typename registers> ParseResult<uarch, registers> parse(const QString &source) {
  ParseResult<uarch, registers> result;
  int startIdx = 0, endIdx = 0, lineNumber = 0, addressCounter = 0;
  do {
    endIdx = source.indexOf('\n', startIdx);
    QStringView line;
    // Allows us to operate without a trailing \n
    if (endIdx == -1) line = QStringView(source).mid(startIdx);
    else line = QStringView(source).mid(startIdx, endIdx - startIdx);
    typename ParseResult<uarch, registers>::Line codeLine;
    QString error;
    if (detail::parseLine<uarch, registers>(line, codeLine, error)) {
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
  } while (endIdx != -1);
  // Populate deferred values for all lines
  for (auto &line : result.program) {
    auto range = line.deferredValues.asKeyValueRange();
    for (auto [signal, symbol] : std::as_const(range)) {
      if (result.symbols.contains(symbol)) {
        auto value = result.symbols[symbol];
        if (value.has_value()) {
          line.controls.set(signal, *value);
          continue;
        }
      }
      result.errors.emplace_back(std::make_pair(line.address, "Undefined symbol: " + symbol));
    }
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
  UnitPre,
  UnitPost,
  Symbol,
  Invalid
};
class TokenBuffer {
public:
  TokenBuffer(const QStringView &line);
  int matchCount() const;
  bool inputRemains() const;
  bool match(Token token, QStringView *out = nullptr);
  bool peek(Token token, QStringView *out = nullptr);
  QStringView rest();
private:
  const QStringView &_data;
  int _start = 0, _end = 0, _matchCount = 0;
  std::optional<Token> _currentToken = std::nullopt;
};

} // namespace detail

template <typename uarch, typename registers>
bool detail::parseLine(const QStringView &line, typename ParseResult<uarch, registers>::Line &code, QString &error) {
  using Line = ParseResult<uarch, registers>::Line;
  int current_group = 0;
  int signals_in_group = 0;
  TokenBuffer buf(line);
  QStringView current;
  while (buf.inputRemains()) {
    if (code.type == Line::Type::Pre || code.type == Line::Type::Post) {
      bool ok;
      int address = 0, value = 0;
      // Rely on operator short-circuiting to ensure that the first match is put into t.
      // Used to decide between hex/decimal values.
      Token t;
      if (!buf.match(Token::Identifier, &current)) return error = "Expected identifier after pre/post unit", false;
      else if (current.compare("mem", Qt::CaseInsensitive) == 0) { // Match Mem[numeric]=value
        if (!buf.match(Token::LeftBracket)) return error = "Expected '[' after 'Mem'", false;
        else if (!buf.match(Token::Hexadecimal, &current)) return error = "Expected address after '['", false;
        else if (address = current.mid(2).toInt(&ok, 16); !ok) return error = "Failed to parse address", false;
        else if (address < 0 || address > 0xFFFF) return error = "Address out of range", false;
        else if (!buf.match(Token::RightBracket)) return error = "Expected ']' after address", false;
        else if (!buf.match(Token::Equals)) return error = "Expected '=' after Mem", false;
        else if (!buf.match(t = Token::Hexadecimal, &current) && !buf.match(t = Token::Decimal, &current))
          return error = "Expected value after '='", false;
        else if (value = current.toInt(&ok, t == Token::Hexadecimal ? 16 : 10); !ok)
          return error = "Failed to parse value", false;
        else if (value < 0 || value > 255) return error = "Value too large", false;
        else code.tests.emplace_back(MemValue{static_cast<quint8>(value), static_cast<quint16>(address)});
      } else { // Match identifer=value
        auto maybe_register = registers::parse_register(current);
        if (!maybe_register.has_value()) return error = "Unknown register: " + current, false;
        else if (!buf.match(Token::Equals)) return error = "Expected '=' after register", false;
        else if (!buf.match(t = Token::Hexadecimal, &current) && !buf.match(t = Token::Decimal, &current))
          return error = "Expected value after '='", false;
        else if (value = current.toInt(&ok, t == Token::Hexadecimal ? 16 : 10); !ok)
          return error = "Failed to parse value", false;
        else if (value < 0 || (1 << 8 * registers::register_byte_size(*maybe_register)) - 1 < value)
          return error = "Register value too large", false;
        else code.tests.emplace_back(RegisterValue<registers>{*maybe_register, static_cast<quint32>(value)});
      }

      if (buf.match(Token::Comma) || buf.match(Token::Empty) || !buf.inputRemains()) continue;
      else if (buf.match(Token::Comment, &current)) code.comment = current.toString();
      else return error = "Unexpected comma, newline, or comment after test", false;
    } else if (buf.matchCount() == 0 && buf.match(Token::UnitPre)) code.type = Line::Type::Pre;
    else if (buf.matchCount() == 0 && buf.match(Token::UnitPost)) code.type = Line::Type::Post;
    else if (buf.match(Token::Symbol, &current)) {
      if (!uarch::allows_symbols()) return error = "Symbols are forbidden", false;
      code.symbolDecl = current.left(current.size() - 1).toString(); // Remove trailing :
      if (!buf.peek(Token::Identifier)) return error = "Expected identifier after symbol declaration", false;
    } else if (buf.match(Token::Semicolon)) {
      current_group++;
      signals_in_group = 0;
      if (current_group >= uarch::max_signal_groups()) return error = "Unexpected semicolon", false;
    } else if (buf.match(Token::Identifier, &current)) {
      auto maybe_signal = uarch::parse_signal(current);
      if (!maybe_signal.has_value()) return error = "Unknown signal: " + current, false;
      typename uarch::Signals s = *maybe_signal;

      if (uarch::is_clock(s)) {
        if (code.controls.enabled(s)) return error = "Clock signal already defined", false;
        else if (auto group = uarch::signal_group(s); group != current_group) {
          if (signals_in_group == 0) current_group = group;
          else return error = "Unexpected signal " + current, false;
        }
        code.controls.set(s, 1), signals_in_group++;
        if (buf.match(Token::Comma) || buf.match(Token::Empty) || !buf.inputRemains()) continue;
        else if (buf.match(Token::Comment, &current)) code.comment = current.toString();
        else if (buf.peek(Token::Semicolon)) continue;
        else return (error = "Unexpected token after clock signal"), false;
      } else {
        if (auto group = uarch::signal_group(s); group != current_group) {
          if (signals_in_group == 0) current_group = group;
          else return error = "Unexpected signal " + current, false;
        }

        if (!buf.match(Token::Equals)) return error = "Expected '=' after signal", false;
        else if (uarch::signal_allows_symbolic_argument(s) && buf.match(Token::Identifier, &current)) {
          code.controls.enable(s); // Ensure that the line is flagged as a code line if this is the only signal.
          code.deferredValues[s] = current.toString();
        } else if (buf.match(Token::Decimal, &current)) {
          bool ok = false;
          if (int value = current.toInt(&ok); !ok) return error = "Failed to parse signal value", false;
          else if (code.controls.enabled(s)) return error = "Signal already defined", false;
          else if ((1 << uarch::signal_bit_size(s)) - 1 < value) return error = "Signal value too large", false;
          else code.controls.set(s, value);
        } else return error = "Expected value for signal", false;

        if (buf.match(Token::Comma) || buf.match(Token::Empty) || !buf.inputRemains()) continue;
        else if (buf.match(Token::Comment, &current)) code.comment = current.toString();
        else if (buf.peek(Token::Semicolon)) continue;
        else return error = "Unexpected token after signal", false;
      }
    } else if (buf.match(Token::Comment, &current)) return code.comment = current.toString(), true;
    else if (buf.match(Token::Empty)) return true;
    else return error = "Unexpected token: " + buf.rest(), false;
  }
  return true;
}
} // namespace pepp::ucode
