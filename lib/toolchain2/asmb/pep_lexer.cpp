#include "./pep_lexer.hpp"
#include <charconv>
#include <regex>
#include <spdlog/spdlog.h>
#include "./pep_tokens.hpp"
#include "core/libs/bitmanip/strings.hpp"

pepp::tc::lex::PepLexer::PepLexer(std::shared_ptr<std::unordered_set<std::string>> identifier_pool,
                                  support::SeekableData &&data)
    : ALexer(identifier_pool, std::move(data)) {}

bool pepp::tc::lex::PepLexer::input_remains() const { return _cursor.input_remains(); }

std::shared_ptr<pepp::tc::lex::Token> pepp::tc::lex::PepLexer::next_token() {
  using LocationInterval = support::LocationInterval;
  static const std::regex identifier("[a-zA-Z_][a-zA-Z0-9_]*");
  static const std::regex macroInvoke("@[a-zA-Z][a-zA-Z0-9_]*");
  static const std::regex directive("\\.[a-zA-Z][a-zA-Z0-9_]*");
  static const std::regex symbol("[a-zA-Z_][a-zA-Z0-9_]*:");
  static const std::regex decimal("[0-9]+");
  static const std::regex hexadecimal("0[xX][0-9a-fA-F]+");
  static const std::regex badHex("0[xX]");
  static const std::regex comment(";[^\n]*");
  static const std::regex charConstant(R"('([^'\\]|\\[bvnrt\\0']|\\[xX][0-9a-fA-F]{2})')");
  static const std::regex strConstant(R"("([^"\\]|\\[bvnrt\\0"]|\\[xX][0-9a-fA-F]{2})*\")");
  std::shared_ptr<pepp::tc::lex::Token> current_token = nullptr;
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
    } else if (next == ',') {
      _cursor.advance(1);
      current_token = std::make_shared<Literal>(LocationInterval{loc_start, _cursor.location()}, ",");
      break;
    } else if (next == '+' || next == '-') {
      auto sign = (next == '-') ? -1 : 1;
      // "Eat" the sign so that we can parse the number after it.
      _cursor.skip(1);
      if (auto maybeDec = _cursor.matchView(decimal); !maybeDec.empty()) {
        using Format = Integer::Format;
        _cursor.advance(maybeDec.size());
        auto text = _cursor.select();
        int val = 0;
        (void)std::from_chars(text.data(), text.data() + text.size(), val, 10);
        auto fmt = sign < 0 ? Format::SignedDec : Format::UnsignedDec;
        current_token = std::make_shared<Integer>(LocationInterval{loc_start, _cursor.location()}, sign * val, fmt);
      } else current_token = std::make_shared<Invalid>(LocationInterval{loc_start, _cursor.location()});
      break;
    } else if (auto maybeSymbol = _cursor.matchView(symbol); !maybeSymbol.empty()) {
      _cursor.advance(maybeSymbol.size());
      auto text = _cursor.select().substr(1); // Drop trailing :
      auto const *id = &*_pool->emplace(text).first;
      current_token = std::make_shared<SymbolDeclaration>(LocationInterval{loc_start, _cursor.location()}, id);
      break;
    } else if (auto maybeIdent = _cursor.matchView(identifier); !maybeIdent.empty()) {
      _cursor.advance(maybeIdent.size());
      auto const *id = &*_pool->emplace(_cursor.select()).first;
      current_token = std::make_shared<Identifier>(LocationInterval{loc_start, _cursor.location()}, id);
      break;
    } else if (auto maybeMacro = _cursor.matchView(macroInvoke); !maybeMacro.empty()) {
      _cursor.advance(maybeMacro.size());
      auto const *id = &*_pool->emplace(_cursor.select().substr(1)).first; // Drop leading @
      current_token = std::make_shared<MacroInvocation>(LocationInterval{loc_start, _cursor.location()}, id);
      break;
    } else if (auto maybeDot = _cursor.matchView(directive); !maybeDot.empty()) {
      _cursor.advance(maybeDot.size());
      auto const *id = &*_pool->emplace(_cursor.select().substr(1)).first; // Drop leading .
      current_token = std::make_shared<DotCommand>(LocationInterval{loc_start, _cursor.location()}, id);
      break;
    } else if (next == '.') { // Bad dot command!
      _cursor.advance(1);
      current_token = std::make_shared<Invalid>(LocationInterval{loc_start, _cursor.location()});
      break;
    } else if (auto maybeHex = _cursor.matchView(hexadecimal); !maybeHex.empty()) {
      using Format = Integer::Format;
      _cursor.advance(maybeHex.size());
      auto text = _cursor.select();
      int val = 0;
      (void)std::from_chars(text.data(), text.data() + text.size(), val, 16);
      current_token = std::make_shared<Integer>(LocationInterval{loc_start, _cursor.location()}, val, Format::Hex);
      break;
    } else if (auto maybeBadHex = _cursor.matchView(badHex); !maybeBadHex.empty()) {
      _cursor.advance(maybeBadHex.size());
      current_token = std::make_shared<Invalid>(LocationInterval{loc_start, _cursor.location()});
      break;
    } else if (auto maybeDec = _cursor.matchView(decimal); !maybeDec.empty()) {
      using Format = Integer::Format;
      _cursor.advance(maybeDec.size());
      auto text = _cursor.select();
      int val = 0;
      (void)std::from_chars(text.data(), text.data() + text.size(), val, 10);
      current_token =
          std::make_shared<Integer>(LocationInterval{loc_start, _cursor.location()}, val, Format::UnsignedDec);
      break;
    } else if (auto maybeComment = _cursor.matchView(comment); !maybeComment.empty()) {
      _cursor.advance(maybeComment.size());
      auto const *id = &*_pool->emplace(_cursor.select().substr(1)).first; // Drop leading ;
      current_token = std::make_shared<InlineComment>(LocationInterval{loc_start, _cursor.location()}, id);
      break;
    } else if (next == '\'') {
      if (auto maybeChar = _cursor.matchView(charConstant); !maybeChar.empty()) {
        _cursor.advance(maybeChar.size());
        // Omit open and close quotes.
        auto text = bits::chopped(_cursor.select().substr(1), 1);
        current_token =
            std::make_shared<CharacterConstant>(LocationInterval{loc_start, _cursor.location()}, std::string{text});
        break;
      } else {
        _cursor.advance(1);
        current_token = std::make_shared<Invalid>(LocationInterval{loc_start, _cursor.location()});
        break;
      }
    } else if (next == '"') {
      if (auto maybeStr = _cursor.matchView(strConstant); !maybeStr.empty()) {
        _cursor.advance(maybeStr.size());
        // Omit open and close quotes.
        auto text = bits::chopped(_cursor.select().substr(1), 1);
        auto const *id = &*_pool->emplace(text).first;
        current_token = std::make_shared<StringConstant>(LocationInterval{loc_start, _cursor.location()}, id);
        break;
      } else {
        _cursor.advance(1);
        current_token = std::make_shared<Invalid>(LocationInterval{loc_start, _cursor.location()});
        break;
      }
    } else {
      _cursor.advance(1);
      current_token = std::make_shared<Invalid>(LocationInterval{loc_start, _cursor.location()});
      break;
    }
  }
  // End must have been all whitespace, treat as empty token.
  if (current_token == nullptr)
    current_token = std::make_shared<Empty>(LocationInterval{loc_start, _cursor.location()});
  notify_listeners(current_token);
  if (print_tokens && current_token) SPDLOG_TRACE("Token: {}", current_token->repr());
  _cursor.skip(0);
  return current_token;
}
