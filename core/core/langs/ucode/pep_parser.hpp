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

#include <core/ds/linenumbers.hpp>
#include <map>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>
#include <vector>
#include "./pep_ir.hpp"
#include "./pep_lexer.hpp"
#include "./pep_tokens.hpp"
#include "core/compile/lex/buffer.hpp"
#include "core/ds/case_insensitive.hpp"
#include "core/math/bitmanip/strings.hpp"

namespace pepp::tc::parse {
// 0-indexed line number and an error message for that line.
using Error = std::pair<int, std::string>;
// All errors encountered while parsing the source program. May have multiple errors per-line.
using Errors = std::vector<Error>;

template <typename uarch, typename registers> struct ParseResult {
  Errors errors;
  // All symbol declarations and their associated addresses.
  // If a symbol is referenced but not defined, it will not be present in the map.
  // This behavior is the opposite of our ASMB symbol table.
  std::map<std::string, uint16_t> symbols;
  using Program = std::vector<pepp::tc::ir::Line<uarch, registers>>;
  Program program;
};

template <typename registers> struct ExtractedTests {
  std::vector<pepp::tc::ir::Test<registers>> pre, post;
};
// The vector of lines produced by parse is hard to execute, since there may be gaps between executable lines.
// This method extract only lines which contain control signals, ignoring comment-only lines, test lines, etc.
// The result is usable by a microcode simulator without any further post-processing.
template <typename uarch, typename registers>
ExtractedTests<registers> tests(const ParseResult<uarch, registers> &result) {
  ExtractedTests<registers> ret;
  for (const auto &line : result.program)
    if (line.type == ir::Line<uarch, registers>::Type::Pre) {
      for (const auto &test : line.tests) ret.pre.emplace_back(test);
    } else if (line.type == ir::Line<uarch, registers>::Type::Post) {
      for (const auto &test : line.tests) ret.post.emplace_back(test);
    }
  return ret;
}

// The vector of lines produced by parse is hard to execute, since there may be gaps between executable lines.
// This method extract only lines which contain control signals, ignoring comment-only lines, test lines, etc.
// The result is usable by a microcode simulator without any further post-processing.
template <typename uarch, typename registers>
std::vector<typename uarch::Code> microcodeFor(const ParseResult<uarch, registers> &result) {
  std::vector<typename uarch::Code> ret;
  for (const auto &line : result.program)
    if (line.type == ir::Line<uarch, registers>::Type::Code && line.controls.enables.any())
      ret.emplace_back(line.controls.code);
  return ret;
}

template <typename uarch, typename registers> bool is_code_line(const typename ir::Line<uarch, registers> &line) {
  return line.type == ir::Line<uarch, registers>::Type::Code && line.controls.enables.any();
}

template <typename uarch, typename registers>
std::vector<typename uarch::CodeWithEnables> microcodeEnableFor(const ParseResult<uarch, registers> &result) {
  std::vector<typename uarch::CodeWithEnables> ret;
  for (const auto &line : result.program)
    if (is_code_line<uarch, registers>(line)) ret.emplace_back(line.controls);
  return ret;
}

template <typename uarch, typename registers>
Line2Address addressesForProgram(const ParseResult<uarch, registers> &result) {
  Line2Address ret;

  for (int line_it = 0; line_it < result.program.size(); line_it++) {
    const auto &line = result.program[line_it];
    if (!is_code_line<uarch, registers>(line)) continue;
    else if (!ret.add_mapping(line_it, line.address)) {
      // Should never be hit since addressed are assigned sequentially.
      SPDLOG_WARN("Duplicate line/address mapping: line {} address {:X}", line_it, line.address);
    }
  }
  return ret;
}

template <typename uarch, typename registers> struct MicroParser {
  MicroParser(std::string &&source, std::shared_ptr<std::unordered_set<std::string>> pool = nullptr)
      : _pool(pool ? pool : std::make_shared<std::unordered_set<std::string>>()),
        _lexer(_pool, support::SeekableData{std::move(source)}), _buf(&_lexer) {};
  // Given some source code, parse it as a microcode program.
  // We assume that not language constructs span multiple lines, so we defer the real parsing work to parseLine.
  // This means parse() is mostly responsible for splitting the source into lines, updating address & symbol values, and
  // associating errors with line numbers.
  ParseResult<uarch, registers> parse();

private:
  bool nextLine(typename ir::Line<uarch, registers> &code, std::string &error);
  std::shared_ptr<std::unordered_set<std::string>> _pool;
  lex::MicroLexer _lexer;
  lex::Buffer _buf;
};

template <typename uarch, typename registers>
inline ParseResult<uarch, registers> MicroParser<uarch, registers>::parse() {
  ParseResult<uarch, registers> result;
  int addressCounter = 0;
  while (_buf.input_remains()) {
    typename ir::Line<uarch, registers> codeLine;
    std::string error;
    if (nextLine(codeLine, error)) {
      // If a line defines control signals, it can be branched to and needs a unique address
      if (codeLine.controls.enables.any()) codeLine.address = addressCounter++;
      // With address assignment complete, we can update the symbol table.
      if (codeLine.symbolDecl.has_value()) {
        auto symbol = *codeLine.symbolDecl;
        if (result.symbols.contains(symbol))
          result.errors.emplace_back(
              std::make_pair(_lexer.current_location().row, "Symbol already defined: " + symbol));
        else result.symbols[symbol] = codeLine.address;
      }
      result.program.emplace_back(codeLine);
    } else {
      auto span = _lexer.synchronize();
      result.errors.emplace_back(std::make_pair(span.lower().row, error));
    }
  }
  // For signals with symbolic values, substitute symbol for integer.
  for (auto &line : result.program) {
    for (auto [signal, symbol] : std::as_const(line.deferredValues)) {
      if (result.symbols.contains(symbol)) line.controls.set(signal, result.symbols[symbol]);
      else result.errors.emplace_back(std::make_pair(line.address, "Undefined symbol: " + symbol));
    }
  }
  return result;
}

template <typename uarch, typename registers>
inline bool MicroParser<uarch, registers>::nextLine(ir::Line<uarch, registers> &code, std::string &error) {
  using namespace tc::lex;
  using MTT = MicrocodeTokenType;
  using CTT = CommonTokenType;
  using Line = ir::Line<uarch, registers>;
  int current_group = 0, signals_in_group = 0;
  Checkpoint cp(_buf);

  while (_buf.input_remains()) {
    if (_buf.count_matched_tokens() == 0 && _buf.match((int)MTT::LineNumber)) continue;
    // If we've already determined that the current line is a test case, enter this special state which
    // handles memory and register tests
    else if (code.type == Line::Type::Pre || code.type == Line::Type::Post) {
      bool ok;
      u32 address = 0, value = 0;
      // Rely on operator short-circuiting to ensure that the first match is put into t.
      // Used to decide how to parse hex/decimal values.
      using ci_sv = std::basic_string_view<char, bts::ci_char_traits>;
      // Nested logical MUST not call continue/return on success. There is shared book keeping logic between tests
      if (auto asId = _buf.match<Identifier>(); !asId) return error = "Expected identifier after pre/post unit", false;
      else if (auto ci = bts::to_ci_stringview(asId->view()); ci.compare("mem") == 0) { // Match Mem[numeric]=value
        using enum Integer::Format;
        if (!_buf.match_literal("[")) return error = "Expected '[' after 'Mem'", false;
        else if (auto asInt = _buf.match<Integer>(); !asInt) return error = "Expected address after '['", false;
        else if (address = asInt->value; asInt->format != Hex) return error = "Failed to parse address", false;
        else if (address > 0xFFFF) return error = "Address out of range", false;
        else if (!_buf.match_literal("]")) return error = "Expected ']' after address", false;
        else if (!_buf.match_literal("=")) return error = "Expected '=' after Mem", false;
        // Short circuiting used to ensure t has the token of the matched value.
        else if (asInt = _buf.match<Integer>(); !asInt) return error = "Expected value after '='", false;
        else if (value = asInt->value;
                 !(asInt->format == Hex || asInt->format == SignedDec || asInt->format == UnsignedDec))
          return error = "Failed to parse value", false;
        else if (value > 0xffff) return error = "Value too large", false;
        else if (value < 256) code.tests.emplace_back(tc::ir::MemTest((u16)address, (u8)value));
        else code.tests.emplace_back(tc::ir::MemTest((u16)address, (u16)value));
      } else { // Match identifer=value for registers
        if (auto maybe_register = registers::parse_register(asId->to_string()); maybe_register.has_value()) {
          using enum Integer::Format;
          if (!_buf.match_literal("=")) return error = "Expected '=' after register", false;
          // Short circuiting used to ensure t has the token of the matched value.
          else if (auto asInt = _buf.match<Integer>(); !asInt) return error = "Expected value after '='", false;
          else if (value = asInt->value;
                   !(asInt->format == Hex || asInt->format == SignedDec || asInt->format == UnsignedDec))
            return error = "Failed to parse value", false;
          else if ((1 << (8 * registers::register_byte_size(*maybe_register))) - 1 < value)
            return error = fmt::format("Register value too large {:X}", value), false;
          else code.tests.emplace_back(tc::ir::RegisterTest<registers>{*maybe_register, static_cast<u32>(value)});
        } else if (auto maybe_csr = registers::parse_csr(asId->to_string()); maybe_csr.has_value()) {
          using enum Integer::Format;
          if (!_buf.match_literal("=")) return error = "Expected '=' after register", false;
          else if (auto asInt = _buf.match<Integer>(); !asInt) return error = "Expected value after '='", false;
          else if (value = asInt->value; asInt->format != UnsignedDec) return error = "Failed to parse value", false;
          else if (value > 1) return error = "Register value too large", false;
          else code.tests.emplace_back(tc::ir::CSRTest<registers>{*maybe_csr, (bool)value});
        } else return error = fmt::format("Unknown register: {}", asId->view()), false;
      }

      // Ensure that each item is followed by some seperator or a comment, prevent constructs like A=7 B=2
      if (_buf.match_literal(",") || !_buf.input_remains()) continue;
      else if (_buf.match<Empty>()) return true;
      else if (auto asCom = _buf.match<InlineComment>(); asCom) code.comment = std::string{asCom->view()};
      else return error = "Unexpected comma, newline, or comment after test", false;
    } else if (_buf.count_matched_tokens() == 0 && _buf.match<UnitPre>()) code.type = Line::Type::Pre;
    else if (_buf.count_matched_tokens() == 0 && _buf.match<UnitPost>()) code.type = Line::Type::Post;
    else if (auto asSym = _buf.match<SymbolDeclaration>(); asSym) {
      if (!uarch::allows_symbols()) return error = "Symbols are forbidden", false;
      auto current = asSym->view();
      code.symbolDecl = bits::chopped(current, 1); // Remove trailing :
      if (!_buf.peek((int)CTT::Identifier)) return error = "Expected identifier after symbol declaration", false;
    } else if (_buf.match_literal(";")) {
      current_group++;
      signals_in_group = 0;
      if (current_group >= uarch::max_signal_groups()) return error = "Unexpected semicolon", false;
    } else if (auto asId = _buf.match<Identifier>(); asId) {
      auto maybe_signal = uarch::parse_signal(asId->to_string());
      if (!maybe_signal.has_value()) return error = fmt::format("Unknown signal: {}", asId->view()), false;
      typename uarch::Signals s = *maybe_signal;
      // If no signals are in this group, allow "jumping" to a higher group.
      // Do not allow jumping backward, and do not allow signals with conflicting groups numbers.
      if (auto group = uarch::signal_group(s); group != current_group) {
        if (signals_in_group == 0 && group >= current_group) current_group = group;
        else return error = fmt::format("Unexpected signal: {}", asId->view()), false;
      }

      // Handle clocks vs control signals.
      if (uarch::is_clock(s)) {
        if (code.controls.enabled(s)) return error = "Clock signal already defined", false;
        code.controls.set(s, 1), signals_in_group++;
      } else {
        using enum Integer::Format;
        // Nested logical MUST not call continue/return on success, even if at EOL.
        if (code.controls.enabled(s)) return error = "Control signal already defined", false;
        else if (!_buf.match_literal("=")) return error = "Expected '=' after signal", false;
        else if (auto asId = _buf.match<Identifier>(); asId) {
          if (uarch::signal_allows_symbolic_argument(s)) {
            code.controls.enable(s); // Ensure that the line is flagged as a code line if this is the only signal.
            code.deferredValues[s] = asId->view();
          } else return error = "Signal does not allow symbolic argument", false;

        } else if (auto i = _buf.match<Integer>(); i && i->format == UnsignedDec) {
          auto value = i->value;
          if ((1 << uarch::signal_bit_size(s)) - 1 < value) return error = "Signal value too large", false;
          else code.controls.set(s, value);
        } else return error = "Expected value for signal", false;
      }

      // Ensure that each item is followed by some seperator or a comment, prevent constructs like A=7 B=2
      if (auto c = _buf.match<InlineComment>(); c) code.comment = c->view();
      else if (_buf.peek_literal(";") || _buf.match_literal(",") || _buf.peek<Empty>() || !_buf.input_remains())
        continue;
      else return (error = "Unexpected  seperator after signal"), false;
    } else if (auto c = _buf.match<InlineComment>()) code.comment = c->view();
    else if (_buf.match<Empty>()) return true;
    else return error = fmt::format("Unexpected token: {}", _buf.peek()->to_string()), false;
  }
  return true;
}

} // namespace pepp::tc::parse
