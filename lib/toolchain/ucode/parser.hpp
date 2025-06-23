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
// A test condition which gets/sets a memory address
struct MemTest {
  MemTest() = default;
  MemTest(quint16 addr, quint16 value);
  MemTest(quint16 addr, quint8 value);
  ~MemTest() = default;
  MemTest(const MemTest &) = default;
  MemTest(MemTest &&) = default;
  MemTest &operator=(const MemTest &) = default;
  MemTest &operator=(MemTest &&) = default;
  quint8 value[2] = {0, 0};
  quint16 address = 0;
  quint8 size = 1; // Either 1 or 2 bytes.
  operator QString() const;
};
// A test condition which gets/sets a variable-sized register
template <typename registers> struct RegisterTest {
  typename registers::NamedRegisters reg;
  // Based on the size of the register, you MUST mask this down to [1-3] bytes.
  // You will also need to byteswap and shift depending on the architecture
  quint32 value = 0;
  operator QString() const {
    const auto size = registers::register_byte_size(reg);
    static const quint32 masks[4] = {0, 0xFF, 0xFF'FF, 0xFF'FF'FF'FF};
    return QString("%1=0x%2")
        .arg(registers::register_name(reg))
        .arg(QString::number(value & masks[size], 16).rightJustified(2 * size, '0'));
  }
};
// A test condition which gets/sets a variable-sized register
template <typename registers> struct CSRTest {
  typename registers::CSRs reg;
  bool value = false;
  operator QString() const { return QString("%1=0x%2").arg(registers::csr_name(reg)).arg(QString::number(value)); }
};

// A UnitPre or UnitPost is a comma-deliited set of memory tests or register tests
template <typename registers> using Test = std::variant<MemTest, RegisterTest<registers>, CSRTest<registers>>;
template <typename registers> QString toString(const Test<registers> &test) {
  if (std::holds_alternative<MemTest>(test)) return std::get<MemTest>(test);
  else if (std::holds_alternative<RegisterTest<registers>>(test)) return std::get<RegisterTest<registers>>(test);
  else if (std::holds_alternative<CSRTest<registers>>(test)) return std::get<CSRTest<registers>>(test);
  else return QString();
}

// 0-indexed line number and an error message for that line.
using Error = std::pair<int, QString>;
// All errors encountered while parsing the source program. May have multiple errors per-line.
using Errors = std::vector<Error>;

template <typename uarch, typename registers> struct ParseResult {
  // Wrap control signals (e.g., our object code) with some assembler IR
  struct Line {
    typename uarch::CodeWithEnables controls;
    std::optional<QString> comment, symbolDecl;
    uint16_t address = -1; // Needed to compute branch targets in control section
    // Some signals allow symbolic values, whose values are only known after parsing completes.
    // This records which signals must be updated after all lines have been parsed.
    QMap<typename uarch::Signals, QString> deferredValues;
    // Will always be code unless this is a UnitPre or UnitPost.
    enum class Type { Code, Pre, Post } type = Type::Code;
    // List of all tests defined in UnitPre/UnitPost. See type to determine which.
    QList<Test<registers>> tests;
  };
  Errors errors;
  // All symbol declarations and their associated addresses.
  // If a symbol is referenced but not defined, it will not be present in the map.
  // This behavior is the opposite of our ASMB symbol table.
  QMap<QString, uint16_t> symbols;
  using Program = std::vector<Line>;
  Program program;
};
// Given an assembled line, produce the cannonical formatted representation of that line.
template <typename uarch, typename registers> QString format(const typename ParseResult<uarch, registers>::Line &line) {
  QString symbolDecl;
  if (line.symbolDecl.has_value()) symbolDecl = *line.symbolDecl + ": ";
  auto _signals = line.controls.toString();
  return QString("%1%2%3").arg(symbolDecl, _signals, line.comment.has_value() ? *line.comment : "");
}
namespace detail {
// Forward declare a helper to do the actual parsing.
template <typename uarch, typename registers>
bool parseLine(const QStringView &line, typename ParseResult<uarch, registers>::Line &code, QString &error);
} // namespace detail

template <typename registers> struct ExtractedTests {
  QList<Test<registers>> pre, post;
};
// The vector of lines produced by parse is hard to execute, since there may be gaps between executable lines.
// This method extract only lines which contain control signals, ignoring comment-only lines, test lines, etc.
// The result is usable by a microcode simulator without any further post-processing.
template <typename uarch, typename registers>
ExtractedTests<registers> tests(const ParseResult<uarch, registers> &result) {
  ExtractedTests<registers> ret;
  for (const auto &line : result.program)
    if (line.type == ParseResult<uarch, registers>::Line::Type::Pre) ret.pre.append(line.tests);
    else if (line.type == ParseResult<uarch, registers>::Line::Type::Post) ret.post.append(line.tests);
  return ret;
}

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

// Given some source code, parse it as a microcode program.
// We assume that not language constructs span multiple lines, so we defer the real parsing work to parseLine.
// This means parse() is mostly responsible for splitting the source into lines, updating address & symbol values, and
// associating errors with line numbers.
template <typename uarch, typename registers> ParseResult<uarch, registers> parse(const QString &source) {
  ParseResult<uarch, registers> result;
  int startIdx = 0, endIdx = 0, lineNumber = 0, addressCounter = 0;
  // Rather than split the source on \n and create a temporary heap-allocated vector, we will iteratively find newlines
  // and take substrings. Use do/while with special handline for endIdx==-1 to allow non-\n terminated programs.
  do {
    QStringView line;
    typename ParseResult<uarch, registers>::Line codeLine;
    QString error;
    endIdx = source.indexOf('\n', startIdx);
    // Allows us to operate without a trailing \n
    if (endIdx == -1) line = QStringView(source).mid(startIdx);
    else line = QStringView(source).mid(startIdx, endIdx - startIdx);
    if (detail::parseLine<uarch, registers>(line, codeLine, error)) {
      // If a line defines control signals, it can be branched to and needs a unique address
      if (codeLine.controls.enables.any()) codeLine.address = addressCounter++;
      // With address assignment complete, we can update the symbol table.
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
  // For signals with symbolic values, substitute symbol for integer.
  for (auto &line : result.program) {
    auto range = line.deferredValues.asKeyValueRange();
    for (auto [signal, symbol] : std::as_const(range)) {
      if (result.symbols.contains(symbol)) line.controls.set(signal, result.symbols[symbol]);
      else result.errors.emplace_back(std::make_pair(line.address, "Undefined symbol: " + symbol));
    }
  }
  return result;
}

namespace detail {
// More or less ripped off from Pep9Suite
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
  LineNumber,
  Invalid
};
// Token buffer which I use to implement recursive descent parsing.
class TokenBuffer {
public:
  TokenBuffer(const QStringView &line);
  // Number of tokens match()'ed so far.
  int matchCount() const;
  bool inputRemains() const;
  // Same as peek(), except it advances _start, clears _currentToken, and increments _matchCount.
  bool match(Token token, QStringView *out = nullptr);
  // Returns true if the next token is token, false otherwise.
  // If true and out!=nullptr, out will contain the data of the token.
  bool peek(Token token, QStringView *out = nullptr);
  inline QStringView rest() { return _data.mid(_end); }
  inline bool atStart() { return _start == 0; }

private:
  const QStringView &_data;
  int _start = 0, _end = 0, _matchCount = 0;
  std::optional<Token> _currentToken = std::nullopt;
};

} // namespace detail

template <typename uarch, typename registers>
bool detail::parseLine(const QStringView &line, typename ParseResult<uarch, registers>::Line &code, QString &error) {
  using namespace Qt::StringLiterals;
  using Line = ParseResult<uarch, registers>::Line;
  int current_group = 0, signals_in_group = 0;
  TokenBuffer buf(line);
  QStringView current;
  while (buf.inputRemains()) {
    if (buf.atStart() && buf.match(Token::LineNumber, &current)) continue;
    // If we've already determined that the current line is a test case, enter this special state which
    // handles memory and register tests
    else if (code.type == Line::Type::Pre || code.type == Line::Type::Post) {
      bool ok;
      quint32 address = 0, value = 0;
      // Rely on operator short-circuiting to ensure that the first match is put into t.
      // Used to decide how to parse hex/decimal values.
      Token t;
      // Nested logical MUST not call continue/return on success. There is shared book keeping logic between tests
      if (!buf.match(Token::Identifier, &current)) return error = "Expected identifier after pre/post unit", false;
      else if (current.compare("mem", Qt::CaseInsensitive) == 0) { // Match Mem[numeric]=value
        if (!buf.match(Token::LeftBracket)) return error = "Expected '[' after 'Mem'", false;
        else if (!buf.match(Token::Hexadecimal, &current)) return error = "Expected address after '['", false;
        else if (address = current.toInt(&ok, 16); !ok) return error = "Failed to parse address", false;
        else if (address > 0xFFFF) return error = "Address out of range", false;
        else if (!buf.match(Token::RightBracket)) return error = "Expected ']' after address", false;
        else if (!buf.match(Token::Equals)) return error = "Expected '=' after Mem", false;
        // Short circuiting used to ensure t has the token of the matched value.
        else if (!buf.match(t = Token::Hexadecimal, &current) && !buf.match(t = Token::Decimal, &current))
          return error = "Expected value after '='", false;
        else if (value = current.toInt(&ok, t == Token::Hexadecimal ? 16 : 10); !ok)
          return error = "Failed to parse value", false;
        else if (value > 0xFFFF) return error = "Value too large", false;
        else if (value < 256) code.tests.emplace_back(MemTest((quint16)address, (quint8)value));
        else code.tests.emplace_back(MemTest((quint16)address, (quint16)value));
      } else { // Match identifer=value for registers
        if (auto maybe_register = registers::parse_register(current); maybe_register.has_value()) {
          if (!buf.match(Token::Equals)) return error = "Expected '=' after register", false;
          // Short circuiting used to ensure t has the token of the matched value.
          else if (!buf.match(t = Token::Hexadecimal, &current) && !buf.match(t = Token::Decimal, &current))
            return error = "Expected value after '='", false;
          else if (value = current.toInt(&ok, t == Token::Hexadecimal ? 16 : 10); !ok)
            return error = "Failed to parse value", false;
          else if ((1 << (8 * registers::register_byte_size(*maybe_register))) - 1 < value)
            return error = "Register value too large" + u"%1"_s.arg(value, 16), false;
          else code.tests.emplace_back(RegisterTest<registers>{*maybe_register, static_cast<quint32>(value)});
        } else if (auto maybe_csr = registers::parse_csr(current); maybe_csr.has_value()) {
          if (!buf.match(Token::Equals)) return error = "Expected '=' after register", false;
          else if (!buf.match(t = Token::Decimal, &current)) return error = "Expected value after '='", false;
          else if (value = current.toInt(&ok); !ok) return error = "Failed to parse value", false;
          else if (value > 1) return error = "Register value too large", false;
          else code.tests.emplace_back(CSRTest<registers>{*maybe_csr, (bool)value});
        } else return error = "Unknown register: " + current, false;
      }

      // Ensure that each item is followed by some seperator or a comment, prevent constructs like A=7 B=2
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
      // If no signals are in this group, allow "jumping" to a higher group.
      // Do not allow jumping backward, and do not allow signals with conflicting groups numbers.
      if (auto group = uarch::signal_group(s); group != current_group) {
        if (signals_in_group == 0 && group >= current_group) current_group = group;
        else return error = "Unexpected signal " + current, false;
      }

      // Handle clocks vs control signals.
      if (uarch::is_clock(s)) {
        if (code.controls.enabled(s)) return error = "Clock signal already defined", false;
        code.controls.set(s, 1), signals_in_group++;
      } else {
        // Nested logical MUST not call continue/return on success, even if at EOL.
        if (code.controls.enabled(s)) return error = "Control signal already defined", false;
        else if (!buf.match(Token::Equals)) return error = "Expected '=' after signal", false;
        else if (uarch::signal_allows_symbolic_argument(s) && buf.match(Token::Identifier, &current)) {
          code.controls.enable(s); // Ensure that the line is flagged as a code line if this is the only signal.
          code.deferredValues[s] = current.toString();
        } else if (buf.match(Token::Decimal, &current)) {
          bool ok = false;
          if (int value = current.toInt(&ok); !ok) return error = "Failed to parse signal value", false;
          else if ((1 << uarch::signal_bit_size(s)) - 1 < value) return error = "Signal value too large", false;
          else code.controls.set(s, value);
        } else return error = "Expected value for signal", false;
      }

      // Ensure that each item is followed by some seperator or a comment, prevent constructs like A=7 B=2
      if (buf.match(Token::Comment, &current)) code.comment = current.toString();
      else if (buf.peek(Token::Semicolon) || buf.match(Token::Comma) || buf.match(Token::Empty) || !buf.inputRemains())
        continue;
      else return (error = "Unexpected  seperator after signal"), false;
    } else if (buf.match(Token::Comment, &current)) return code.comment = current.toString(), true;
    else if (buf.match(Token::Empty)) return true;
    else return error = "Unexpected token: " + buf.rest(), false;
  }
  return true;
}
} // namespace pepp::ucode
