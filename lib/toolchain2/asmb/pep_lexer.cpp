#include "./pep_lexer.hpp"
#include <QRegularExpression>
#include "../support/lex/tokens.hpp"
#include "./pep_tokens.hpp"

pepp::tc::lex::PepLexer::PepLexer(std::shared_ptr<support::StringPool> identifier_pool, support::SeekableData &&data)
    : ALexer(identifier_pool, std::move(data)) {}

bool pepp::tc::lex::PepLexer::input_remains() const { return _cursor.input_remains(); }

std::shared_ptr<pepp::tc::lex::Token> pepp::tc::lex::PepLexer::next_token() {
  using LocationInterval = support::LocationInterval;
  static const QRegularExpression identifier("[a-zA-Z_][a-zA-Z0-9_]*");
  static const QRegularExpression macroInvoke("@[a-zA-Z][a-zA-Z0-9_]*");
  static const QRegularExpression directive("\\.[a-zA-Z][a-zA-Z0-9_]*");
  static const QRegularExpression symbol("[a-zA-Z_][a-zA-Z0-9_]*:");
  static const QRegularExpression decimal("[0-9]+");
  static const QRegularExpression hexadecimal("0[xX][0-9a-fA-F]+");
  static const QRegularExpression badHex("0[xX]");
  static const QRegularExpression comment(";[^\n]*");
  static const QRegularExpression charConstant(R"('([^'\\]|\\[bvnrt\\0']|\\[xX][0-9a-fA-F]{2})')");
  static const QRegularExpression strConstant(R"("([^"\\]|\\[bvnrt\\0"]|\\[xX][0-9a-fA-F]{2})*\")");
  std::shared_ptr<pepp::tc::lex::Token> current_token = nullptr;
  auto loc_start = _cursor.location();
  while (input_remains()) {
    auto next = _cursor.peek();
    if (next == "\n") {
      _cursor.advance(1);
      _cursor.newline();
      current_token = std::make_shared<Empty>(LocationInterval{loc_start, _cursor.location()});
      break;
    } else if (next == "\r") {
      _cursor.skip(1);
      loc_start = _cursor.location();
      current_token = std::make_shared<Empty>(LocationInterval{loc_start, _cursor.location()});
      continue;
    } else if (next.isSpace()) {
      _cursor.skip(1);
      loc_start = _cursor.location();
      continue;
    } else if (next == ",") {
      _cursor.advance(1);
      current_token = std::make_shared<Literal>(LocationInterval{loc_start, _cursor.location()}, ",");
      break;
    } else if (next == "+" || next == "-") {
      auto sign = (next == "-") ? -1 : 1;
      // "Eat" the sign so that we can parse the number after it.
      _cursor.skip(1);
      if (auto maybeDec = _cursor.matchView(decimal); maybeDec.hasMatch()) {
        using Format = Integer::Format;
        _cursor.advance(maybeDec.capturedLength(0));
        auto text = _cursor.select();
        auto val = text.toInt(nullptr, 10);
        auto fmt = sign < 0 ? Format::SignedDec : Format::UnsignedDec;
        current_token = std::make_shared<Integer>(LocationInterval{loc_start, _cursor.location()}, sign * val, fmt);
      } else current_token = std::make_shared<Invalid>(LocationInterval{loc_start, _cursor.location()});
      break;
    } else if (auto maybeSymbol = _cursor.matchView(symbol); maybeSymbol.hasMatch()) {
      _cursor.advance(maybeSymbol.capturedLength(0));
      auto text = _cursor.select().chopped(1); // Drop trailing :
      auto id = _pool->insert(text);
      current_token =
          std::make_shared<SymbolDeclaration>(LocationInterval{loc_start, _cursor.location()}, _pool.get(), id);
      break;
    } else if (auto maybeIdent = _cursor.matchView(identifier); maybeIdent.hasMatch()) {
      _cursor.advance(maybeIdent.capturedLength(0));
      auto id = _pool->insert(_cursor.select());
      current_token = std::make_shared<Identifier>(LocationInterval{loc_start, _cursor.location()}, _pool.get(), id);
      break;
    } else if (auto maybeMacro = _cursor.matchView(macroInvoke); maybeMacro.hasMatch()) {
      _cursor.advance(maybeMacro.capturedLength(0));
      auto id = _pool->insert(_cursor.select().mid(1)); // Drop leading @
      current_token =
          std::make_shared<MacroInvocation>(LocationInterval{loc_start, _cursor.location()}, _pool.get(), id);
      break;
    } else if (auto maybeDot = _cursor.matchView(directive); maybeDot.hasMatch()) {
      _cursor.advance(maybeDot.capturedLength(0));
      auto id = _pool->insert(_cursor.select().mid(1)); // Drop leading .
      current_token = std::make_shared<DotCommand>(LocationInterval{loc_start, _cursor.location()}, _pool.get(), id);
      break;
    } else if (next == ".") { // Bad dot command!
      _cursor.advance(1);
      current_token = std::make_shared<Invalid>(LocationInterval{loc_start, _cursor.location()});
      break;
    } else if (auto maybeHex = _cursor.matchView(hexadecimal); maybeHex.hasMatch()) {
      using Format = Integer::Format;
      _cursor.advance(maybeHex.capturedLength(0));
      auto text = _cursor.select();
      auto val = text.toInt(nullptr, 16);
      current_token = std::make_shared<Integer>(LocationInterval{loc_start, _cursor.location()}, val, Format::Hex);
      break;
    } else if (auto maybeBadHex = _cursor.matchView(badHex); maybeBadHex.hasMatch()) {
      _cursor.advance(maybeBadHex.capturedLength(0));
      current_token = std::make_shared<Invalid>(LocationInterval{loc_start, _cursor.location()});
      break;
    } else if (auto maybeDec = _cursor.matchView(decimal); maybeDec.hasMatch()) {
      using Format = Integer::Format;
      _cursor.advance(maybeDec.capturedLength(0));
      auto text = _cursor.select();
      auto val = text.toInt(nullptr, 10);
      current_token =
          std::make_shared<Integer>(LocationInterval{loc_start, _cursor.location()}, val, Format::UnsignedDec);
      break;
    } else if (auto maybeComment = _cursor.matchView(comment); maybeComment.hasMatch()) {
      _cursor.advance(maybeComment.capturedLength(0));
      // Skip over the leading ;
      auto id = _pool->insert(_cursor.select().mid(1));
      current_token = std::make_shared<InlineComment>(LocationInterval{loc_start, _cursor.location()}, _pool.get(), id);
      break;
    } else if (next == "'") {
      if (auto maybeChar = _cursor.matchView(charConstant); maybeChar.hasMatch()) {
        _cursor.advance(maybeChar.capturedLength(0));
        // Omit open and close quotes.
        auto text = _cursor.select().mid(1).chopped(1);
        current_token =
            std::make_shared<CharacterConstant>(LocationInterval{loc_start, _cursor.location()}, text.toString());
        break;
      } else {
        _cursor.advance(1);
        current_token = std::make_shared<Invalid>(LocationInterval{loc_start, _cursor.location()});
        break;
      }
    } else if (next == "\"") {
      if (auto maybeStr = _cursor.matchView(strConstant); maybeStr.hasMatch()) {
        _cursor.advance(maybeStr.capturedLength(0));
        // Omit open and close quotes.
        auto text = _cursor.select().mid(1).chopped(1);
        auto id = _pool->insert(text);
        current_token =
            std::make_shared<StringConstant>(LocationInterval{loc_start, _cursor.location()}, _pool.get(), id);
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
  if (print_tokens && current_token) qDebug().noquote().nospace() << "Token:" << current_token->repr();
  _cursor.skip(0);
  return current_token;
}
