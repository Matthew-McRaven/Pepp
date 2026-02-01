#include "./pep_lexer.hpp"
#include <../../../core/core/ds/case_insensitive.hpp>
#include <charconv>
#include <regex>
#include <spdlog/spdlog.h>
#include "./pep_tokens.hpp"
#include "core/math/bitmanip/strings.hpp"
#include "core/compile/lex/tokens.hpp"

pepp::tc::lex::MicroLexer::MicroLexer(std::shared_ptr<std::unordered_set<std::string>> identifier_pool,
                                      support::SeekableData &&data)
    : ALexer(identifier_pool, std::move(data)) {}

bool pepp::tc::lex::MicroLexer::input_remains() const { return _cursor.input_remains(); }

std::shared_ptr<pepp::tc::lex::Token> pepp::tc::lex::MicroLexer::next_token() {
  using LocationInterval = support::LocationInterval;
  static const std::regex lineNum("[0-9]+\\.");
  static const std::regex identifier("[a-zA-Z_][a-zA-Z0-9_]*");
  static const std::regex symbol("[a-zA-Z_][a-zA-Z0-9_]*:");
  static const std::regex decimal("[0-9]+");
  static const std::regex hexadecimal("0[xX][0-9a-fA-F]+");
  static const std::regex comment("//[^\n]*");
  std::shared_ptr<pepp::tc::lex::Token> current_token;
  auto loc_start = _cursor.location();
  while (input_remains()) {
    auto next = _cursor.peek();
    if (next == '\n') {
      _cursor.advance(1);
      _cursor.newline();
      current_token = std::make_shared<Empty>(LocationInterval{loc_start, _cursor.location()});
      break;
    } else if (next == '\r') {
      _cursor.skip(1);
      loc_start = _cursor.location();
      current_token = std::make_shared<Empty>(LocationInterval{loc_start, _cursor.location()});
      continue;
    } else if (std::isspace(next)) {
      _cursor.skip(1);
      loc_start = _cursor.location();
      continue;
    } else if (next == '[') {
      _cursor.advance(1);
      current_token = std::make_shared<Literal>(LocationInterval{loc_start, _cursor.location()}, "[");
      break;
    } else if (next == ']') {
      _cursor.advance(1);
      current_token = std::make_shared<Literal>(LocationInterval{loc_start, _cursor.location()}, "]");
      break;
    } else if (next == '=') {
      _cursor.advance(1);
      current_token = std::make_shared<Literal>(LocationInterval{loc_start, _cursor.location()}, "=");
      break;
    } else if (next == ',') {
      _cursor.advance(1);
      current_token = std::make_shared<Literal>(LocationInterval{loc_start, _cursor.location()}, ",");
      break;
    } else if (next == ';') {
      _cursor.advance(1);
      current_token = std::make_shared<Literal>(LocationInterval{loc_start, _cursor.location()}, ";");
      break;
    } else if (auto maybeSymbol = _cursor.matchView(symbol); !maybeSymbol.empty()) {
      auto match = maybeSymbol.str(0);
      _cursor.advance(match.size());
      auto const *id = &*_pool->emplace(match).first;
      using ci_sv = std::basic_string_view<char, bts::ci_char_traits>;
      static constexpr const ci_sv UnitPreStr{"UnitPre:"};
      static constexpr const ci_sv UnitPostStr{"UnitPost:"};
      auto sv = ci_sv{id->data(), id->size()};
      // UnitPre and UnitPost are constructs that look like symbols. Hijacking symbol code to avoid 2 extra regexs
      if (sv.compare(UnitPreStr) == 0)
        current_token = std::make_shared<UnitPre>(LocationInterval{loc_start, _cursor.location()});
      else if (sv.compare(UnitPostStr) == 0)
        current_token = std::make_shared<UnitPost>(LocationInterval{loc_start, _cursor.location()});
      else current_token = std::make_shared<SymbolDeclaration>(LocationInterval{loc_start, _cursor.location()}, id);
      break;
    } else if (auto maybeIdent = _cursor.matchView(identifier); !maybeIdent.empty()) {
      auto match = maybeIdent.str(0);
      _cursor.advance(match.size());
      auto const *id = &*_pool->emplace(match).first;
      current_token = std::make_shared<Identifier>(LocationInterval{loc_start, _cursor.location()}, id);
      break;
    } else if (auto maybeLine = _cursor.matchView(lineNum); !maybeLine.empty()) {
      auto match = maybeLine.str(0);
      _cursor.advance(match.size());
      // Remove trailing period, else parsing fails.
      auto text = bits::chopped(match, 1);
      int val = 0;
      (void)std::from_chars(text.data(), text.data() + text.size(), val, 10);
      current_token = std::make_shared<LineNumber>(LocationInterval{loc_start, _cursor.location()}, val);
      break;
    } else if (auto maybeHex = _cursor.matchView(hexadecimal); !maybeHex.empty()) {
      using Format = Integer::Format;
      auto match = maybeHex.str(0);
      _cursor.advance(match.size());
      match = match.substr(2); // Drop 0x
      int val = 0;
      (void)std::from_chars(match.data(), match.data() + match.size(), val, 16);
      current_token = std::make_shared<Integer>(LocationInterval{loc_start, _cursor.location()}, val, Format::Hex);
      break;
    } else if (auto maybeDec = _cursor.matchView(decimal); !maybeDec.empty()) {
      using Format = Integer::Format;
      auto match = maybeDec.str(0);
      _cursor.advance(match.size());
      int val = 0;
      (void)std::from_chars(match.data(), match.data() + match.size(), val, 10);
      auto fmt = val < 0 ? Format::SignedDec : Format::UnsignedDec;
      current_token = std::make_shared<Integer>(LocationInterval{loc_start, _cursor.location()}, val, fmt);
      break;
    } else if (auto maybeComment = _cursor.matchView(comment); !maybeComment.empty()) {
      auto match = maybeComment.str(0);
      _cursor.advance(match.size());
      auto const *id = &*_pool->emplace(match).first;
      current_token = std::make_shared<InlineComment>(LocationInterval{loc_start, _cursor.location()}, id);
      break;
    } else {
      _cursor.advance(1);
      current_token = std::make_shared<Invalid>(LocationInterval{loc_start, _cursor.location()});
      break;
    }
  }
  notify_listeners(current_token);
  if (print_tokens && current_token) SPDLOG_TRACE("Token: {}", current_token->repr());
  _cursor.skip(0);
  return current_token;
}
