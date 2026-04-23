#include "./asmb_lexer.hpp"
#include <charconv>
#include <regex>
#include <spdlog/spdlog.h>
#include "core/compile/lex/tokens.hpp"
#include "core/langs/asmb/asmb_lexer.hpp"
#include "core/langs/asmb/asmb_tokens.hpp"
#include "core/math/bitmanip/strings.hpp"

pepp::tc::lex::AsmbLexer::AsmbLexer(std::shared_ptr<std::unordered_set<std::string>> identifier_pool,
                                    support::SeekableData &&data, AsmbOptions options)
    : ALexer(identifier_pool, std::move(data)), _opts(options) {
  _lineCommentRegex = std::make_unique<std::regex>(options.line_comment_leader + "[^\n]*");
}

bool pepp::tc::lex::AsmbLexer::input_remains() const { return _cursor.input_remains(); }

std::string_view pepp::tc::lex::AsmbLexer::view(support::LocationInterval loc) const {
  const auto start_row = loc.lower().row, end_row = loc.upper().row;
  const auto maybe_start = _row_to_streampos.find(start_row), maybe_end = _row_to_streampos.find(end_row);
  if (maybe_start == _row_to_streampos.end() || maybe_end == _row_to_streampos.end()) return std::string_view{};
  const auto start_pos = maybe_start->second + loc.lower().column, end_pos = maybe_end->second + loc.upper().column;
  return _cursor.view_between(start_pos, end_pos);
}

static const std::string identifier_dot_at_str = R"([@a-zA-Z_][a-zA-Z0-9_.]*)";
static const std::string identifier_nodot_at_str = R"([@a-zA-Z_][a-zA-Z0-9_]*)";
static const std::string identifier_dot_noat_str = R"([a-zA-Z_][a-zA-Z0-9_.]*)";
static const std::string identifier_nodot_noat_str = R"([a-zA-Z_][a-zA-Z0-9_]*)";

std::shared_ptr<pepp::tc::lex::Token> pepp::tc::lex::AsmbLexer::next_token() {
  using LocationInterval = support::LocationInterval;
  static const std::regex identifier_dot_at(identifier_dot_at_str);
  static const std::regex identifier_nodot_at(identifier_nodot_at_str);
  static const std::regex identifier_dot_noat(identifier_dot_noat_str);
  static const std::regex identifier_nodot_noat(identifier_nodot_noat_str);
  const std::regex &identifier = _opts.allow_dot_in_ident
                                     ? (_opts.allow_at_in_ident ? identifier_dot_at : identifier_dot_noat)
                                     : (_opts.allow_at_in_ident ? identifier_nodot_at : identifier_nodot_noat);
  static const std::regex directive("\\.[a-zA-Z][a-zA-Z0-9_]*");
  static const std::regex symbol("[a-zA-Z_][a-zA-Z0-9_]*:");
  static const std::regex decimal("[0-9]+");
  static const std::regex hexadecimal("0[xX][0-9a-fA-F]+");
  static const std::regex badHex("0[xX]");
  static const std::regex charConstant(R"('([^'\\]|\\[bvnrt\\0']|\\[xX][0-9a-fA-F]{2})')");
  static const std::regex strConstant(R"("([^"\\]|\\[bvnrt\\0"]|\\[xX][0-9a-fA-F]{2})*\")");
  std::shared_ptr<pepp::tc::lex::Token> current_token = nullptr;
  auto loc_start = _cursor.location();
  // Ensure when a token starts a newline that we update row:location mapping.
  if (loc_start.column == 0 && !_row_to_streampos.contains(loc_start.row))
    _row_to_streampos[loc_start.row] = _cursor.end();

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
        auto match = maybeDec.str(0);
        using Format = Integer::Format;
        _cursor.advance(match.size());
        int val = 0;
        (void)std::from_chars(match.data(), match.data() + match.size(), val, 10);
        auto fmt = sign < 0 ? Format::SignedDec : Format::UnsignedDec;
        current_token = std::make_shared<Integer>(LocationInterval{loc_start, _cursor.location()}, sign * val, fmt);
      } else
        current_token =
            std::make_shared<Invalid>(LocationInterval{loc_start, _cursor.location()}, std::string{_cursor.select()});
      break;
    } else if (next == '(' && _opts.allow_parens) {
      _cursor.advance(1);
      current_token = std::make_shared<Literal>(LocationInterval{loc_start, _cursor.location()}, "(");
      break;
    } else if (next == ')' && _opts.allow_parens) {
      _cursor.advance(1);
      current_token = std::make_shared<Literal>(LocationInterval{loc_start, _cursor.location()}, ")");
      break;
    } else if (auto maybeSymbol = _cursor.matchView(symbol); !maybeSymbol.empty()) {
      auto match = maybeSymbol.str(0);
      _cursor.advance(match.size());
      auto const *id = &*_pool->emplace(bits::chopped(match, 1)).first; // Drop trailing :
      current_token = std::make_shared<SymbolDeclaration>(LocationInterval{loc_start, _cursor.location()}, id);
      break;
    } else if (auto maybeIdent = _cursor.matchView(identifier); !maybeIdent.empty()) {
      auto match = maybeIdent.str(0);
      _cursor.advance(match.size());
      auto const *id = &*_pool->emplace(match).first;
      current_token = std::make_shared<Identifier>(LocationInterval{loc_start, _cursor.location()}, id);
      break;
    } else if (auto maybeDot = _cursor.matchView(directive); !maybeDot.empty()) {
      auto match = maybeDot.str(0);
      _cursor.advance(match.size());
      auto const *id = &*_pool->emplace(match.substr(1)).first; // Drop leading .
      current_token = std::make_shared<DotCommand>(LocationInterval{loc_start, _cursor.location()}, id);
      break;
    } else if (next == '.') { // Bad dot command!
      _cursor.advance(1);
      current_token =
          std::make_shared<Invalid>(LocationInterval{loc_start, _cursor.location()}, std::string{_cursor.select()});
      break;
    } else if (auto maybeHex = _cursor.matchView(hexadecimal); !maybeHex.empty()) {
      auto match = maybeHex.str(0);
      using Format = Integer::Format;
      _cursor.advance(match.size());
      match = match.substr(2); // drop leading 0x.
      int val = 0;
      (void)std::from_chars(match.data(), match.data() + match.size(), val, 16);
      current_token = std::make_shared<Integer>(LocationInterval{loc_start, _cursor.location()}, val, Format::Hex);
      break;
    } else if (auto maybeBadHex = _cursor.matchView(badHex); !maybeBadHex.empty()) {
      auto match = maybeBadHex.str(0);
      _cursor.advance(match.size());
      current_token =
          std::make_shared<Invalid>(LocationInterval{loc_start, _cursor.location()}, std::string{_cursor.select()});
      break;
    } else if (auto maybeDec = _cursor.matchView(decimal); !maybeDec.empty()) {
      using Format = Integer::Format;
      auto match = maybeDec.str(0);
      _cursor.advance(match.size());
      int val = 0;
      (void)std::from_chars(match.data(), match.data() + match.size(), val, 10);
      current_token =
          std::make_shared<Integer>(LocationInterval{loc_start, _cursor.location()}, val, Format::UnsignedDec);
      break;
    } else if (auto maybeComment = _cursor.matchView(*_lineCommentRegex); !maybeComment.empty()) {
      auto match = maybeComment.str(0);
      _cursor.advance(match.size());
      auto const *id = &*_pool->emplace(match.substr(1)).first; // Drop leading ;
      current_token = std::make_shared<InlineComment>(LocationInterval{loc_start, _cursor.location()}, id);
      break;
    } else if (next == '\'') {
      if (auto maybeChar = _cursor.matchView(charConstant); !maybeChar.empty()) {
        auto match = maybeChar.str(0);
        _cursor.advance(match.size());
        // Omit open and close quotes.
        match = bits::chopped(match.substr(1), 1);
        current_token =
            std::make_shared<CharacterConstant>(LocationInterval{loc_start, _cursor.location()}, std::string{match});
        break;
      } else {
        _cursor.advance(1);
        current_token =
            std::make_shared<Invalid>(LocationInterval{loc_start, _cursor.location()}, std::string{_cursor.select()});
        break;
      }
    } else if (next == '"') {
      if (auto maybeStr = _cursor.matchView(strConstant); !maybeStr.empty()) {
        auto match = maybeStr.str(0);
        _cursor.advance(match.size());
        // Omit open and close quotes.
        match = bits::chopped(match.substr(1), 1);
        auto const *id = &*_pool->emplace(match).first;
        current_token = std::make_shared<StringConstant>(LocationInterval{loc_start, _cursor.location()}, id);
        break;
      } else {
        _cursor.advance(1);
        current_token =
            std::make_shared<Invalid>(LocationInterval{loc_start, _cursor.location()}, std::string{_cursor.select()});
        break;
      }
    } else {
      _cursor.advance(1);
      current_token =
          std::make_shared<Invalid>(LocationInterval{loc_start, _cursor.location()}, std::string{_cursor.select()});
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
