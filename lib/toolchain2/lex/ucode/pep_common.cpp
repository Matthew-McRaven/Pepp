#include "./pep_common.hpp"
#include <QRegularExpression>
#include "../core/common_tokens.hpp"

pepp::tc::lex::LineNumber::LineNumber(support::LocationInterval loc, int lineNo) : Token(loc), line(lineNo) {}

int pepp::tc::lex::LineNumber::type() const { return TYPE; }

QString pepp::tc::lex::LineNumber::type_name() const { return "LineNumber"; }

QString pepp::tc::lex::LineNumber::to_string() const { return QStringLiteral("%1.").arg(line); }

QString pepp::tc::lex::LineNumber::repr() const { return QStringLiteral("LineNumber(%1)").arg(line); }

pepp::tc::lex::UnitPre::UnitPre(support::LocationInterval loc) : Token(loc) {}

int pepp::tc::lex::UnitPre::type() const { return TYPE; }

QString pepp::tc::lex::UnitPre::type_name() const { return "UnitPre"; }

QString pepp::tc::lex::UnitPre::to_string() const { return "UnitPre:"; }

QString pepp::tc::lex::UnitPre::repr() const { return "UnitPre()"; }

pepp::tc::lex::UnitPost::UnitPost(support::LocationInterval loc) : Token(loc) {}

int pepp::tc::lex::UnitPost::type() const { return TYPE; }

QString pepp::tc::lex::UnitPost::type_name() const { return "UnitPost"; }

QString pepp::tc::lex::UnitPost::to_string() const { return "UnitPost:"; }

QString pepp::tc::lex::UnitPost::repr() const { return "UnitPost()"; }

pepp::tc::lex::MicroLexer::MicroLexer(std::shared_ptr<support::StringPool> identifier_pool,
                                      support::SeekableData &&data)
    : ALexer(identifier_pool, std::move(data)) {}

bool pepp::tc::lex::MicroLexer::input_remains() const { return _cursor.input_remains(); }

std::shared_ptr<pepp::tc::lex::Token> pepp::tc::lex::MicroLexer::next_token() {
  using LocationInterval = support::LocationInterval;
  static const QRegularExpression lineNum("[0-9]+\\.");
  static const QRegularExpression identifier("[a-zA-Z][a-zA-Z0-9_]*");
  static const QRegularExpression symbol("[a-zA-Z_][a-zA-Z0-9_]*:");
  static const QRegularExpression decimal("[0-9]+");
  static const QRegularExpression hexadecimal("0[xX][0-9a-fA-F]+");
  static const QRegularExpression comment("//[^\n]*");
  std::shared_ptr<pepp::tc::lex::Token> current_token;
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
    } else if (next == "[") {
      _cursor.advance(1);
      current_token = std::make_shared<Literal>(LocationInterval{loc_start, _cursor.location()}, "[");
      break;
    } else if (next == "]") {
      _cursor.advance(1);
      current_token = std::make_shared<Literal>(LocationInterval{loc_start, _cursor.location()}, "]");
      break;
    } else if (next == "=") {
      _cursor.advance(1);
      current_token = std::make_shared<Literal>(LocationInterval{loc_start, _cursor.location()}, "=");
      break;
    } else if (next == ",") {
      _cursor.advance(1);
      current_token = std::make_shared<Literal>(LocationInterval{loc_start, _cursor.location()}, ",");
      break;
    } else if (next == ";") {
      _cursor.advance(1);
      current_token = std::make_shared<Literal>(LocationInterval{loc_start, _cursor.location()}, ";");
      break;
    } else if (auto maybeSymbol = _cursor.matchView(symbol); maybeSymbol.hasMatch()) {
      _cursor.advance(maybeSymbol.capturedLength(0));
      auto id = _pool->insert(_cursor.select());
      // UnitPre and UnitPost are constructs that look like symbols. Hijacking symbol code to avoid 2 extra regexs
      if (maybeSymbol.capturedView(0).compare("UnitPre:", Qt::CaseInsensitive) == 0)
        current_token = std::make_shared<UnitPre>(LocationInterval{loc_start, _cursor.location()});
      else if (maybeSymbol.capturedView(0).compare("UnitPost:", Qt::CaseInsensitive) == 0)
        current_token = std::make_shared<UnitPost>(LocationInterval{loc_start, _cursor.location()});
      else
        current_token =
            std::make_shared<SymbolDeclaration>(LocationInterval{loc_start, _cursor.location()}, _pool.get(), id);
      break;
    } else if (auto maybeIdent = _cursor.matchView(identifier); maybeIdent.hasMatch()) {
      _cursor.advance(maybeIdent.capturedLength(0));
      auto id = _pool->insert(_cursor.select());
      current_token = std::make_shared<Identifier>(LocationInterval{loc_start, _cursor.location()}, _pool.get(), id);
      break;
    } else if (auto maybeLine = _cursor.matchView(lineNum); maybeLine.hasMatch()) {
      _cursor.advance(maybeLine.capturedLength(0));
      // Remove trailing period, else parsing fails.
      auto text = _cursor.select().chopped(1);
      auto lineNo = text.toInt(nullptr, 10);
      current_token = std::make_shared<LineNumber>(LocationInterval{loc_start, _cursor.location()}, lineNo);
      break;
    } else if (auto maybeHex = _cursor.matchView(hexadecimal); maybeHex.hasMatch()) {
      using Format = Integer::Format;
      _cursor.advance(maybeHex.capturedLength(0));
      auto text = _cursor.select();
      auto val = text.toInt(nullptr, 16);
      current_token = std::make_shared<Integer>(LocationInterval{loc_start, _cursor.location()}, val, Format::Hex);
      break;
    } else if (auto maybeDec = _cursor.matchView(decimal); maybeDec.hasMatch()) {
      using Format = Integer::Format;
      _cursor.advance(maybeDec.capturedLength(0));
      auto text = _cursor.select();
      auto val = text.toInt(nullptr, 10);
      auto fmt = val < 0 ? Format::SignedDec : Format::UnsignedDec;
      current_token = std::make_shared<Integer>(LocationInterval{loc_start, _cursor.location()}, val, fmt);
      break;
    } else if (auto maybeComment = _cursor.matchView(comment); maybeComment.hasMatch()) {
      _cursor.advance(maybeComment.capturedLength(0));
      auto id = _pool->insert(_cursor.select());
      current_token = std::make_shared<InlineComment>(LocationInterval{loc_start, _cursor.location()}, _pool.get(), id);
      break;
    } else {
      _cursor.advance(1);
      current_token = std::make_shared<Invalid>(LocationInterval{loc_start, _cursor.location()});
      break;
    }
  }
  notify_listeners(current_token);
  if (print_tokens && current_token) qDebug().noquote().nospace() << "Token:" << current_token->repr();
  _cursor.skip(0);
  return current_token;
}

pepp::tc::support::LocationInterval pepp::tc::lex::MicroLexer::synchronize() {
  auto start = _cursor.location();
  while (input_remains()) {
    if (auto next = _cursor.peek(); next == "\n") {
      _cursor.advance(1);
      _cursor.newline();
      break;
    } else _cursor.advance(1);
  }
  return {start, _cursor.location()};
}
