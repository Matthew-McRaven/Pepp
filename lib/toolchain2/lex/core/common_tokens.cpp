#include "./common_tokens.hpp"

pepp::tc::lex::Token::Token(support::LocationInterval loc) : _loc(loc) {}
pepp::tc::lex::Token::~Token() {};

QString pepp::tc::lex::Token::to_string() const { return ""; }

QString pepp::tc::lex::Token::repr() const { return QStringLiteral("%1(%2)").arg(type_name(), to_string()); }

bool pepp::tc::lex::Token::mask(int mask) const { return type() & mask; }

pepp::tc::support::LocationInterval pepp::tc::lex::Token::location() const { return _loc; }

pepp::tc::support::Location pepp::tc::lex::Token::start() const { return _loc.lower(); }

pepp::tc::support::Location pepp::tc::lex::Token::end() const { return _loc.upper(); }

pepp::tc::lex::Invalid::Invalid(support::LocationInterval loc) : Token(loc) {}

int pepp::tc::lex::Invalid::type() const { return TYPE; }

QString pepp::tc::lex::Invalid::type_name() const { return "Invalid"; }

pepp::tc::lex::EoF::EoF(support::LocationInterval loc) : Token(loc) {}

int pepp::tc::lex::EoF::type() const { return TYPE; }

QString pepp::tc::lex::EoF::type_name() const { return "EoF"; }

pepp::tc::lex::Empty::Empty(support::LocationInterval loc) : Token(loc) {}

int pepp::tc::lex::Empty::type() const { return TYPE; }

QString pepp::tc::lex::Empty::type_name() const { return "Empty"; }

QString pepp::tc::lex::Empty::to_string() const { return "\n"; }

QString pepp::tc::lex::Empty::repr() const { return "Empty()"; }

pepp::tc::lex::Integer::Integer(support::LocationInterval loc, uint64_t val, Format fmt)
    : Token(loc), format(fmt), value(val) {}

int pepp::tc::lex::Integer::type() const { return TYPE; }

QString pepp::tc::lex::Integer::type_name() const { return "Integer"; }

QString pepp::tc::lex::Integer::to_string() const {
  switch (format) {
  case Format::SignedDec: return QString::number((int64_t)value);
  case Format::UnsignedDec: return QString::number(value);
  case Format::Hex: return "0x" + QString::number(value, 16).toUpper();
  case Format::Bin: return "0b" + QString::number(value, 2);
  default: throw std::logic_error("Unrecognized Integer::Format");
  }
}

pepp::tc::lex::Identifier::Identifier(support::LocationInterval loc, support::StringPool *pool,
                                      support::PooledString id)
    : Token(loc), pool(pool), id(id) {}

int pepp::tc::lex::Identifier::type() const { return TYPE; }

QStringView pepp::tc::lex::Identifier::view() const { return pool->find(id).value(); }

QString pepp::tc::lex::Identifier::type_name() const { return "Identifier"; }

QString pepp::tc::lex::Identifier::to_string() const { return view().toString(); }

pepp::tc::lex::SymbolDeclaration::SymbolDeclaration(support::LocationInterval loc, support::StringPool *pool,
                                                    support::PooledString id)
    : Identifier(loc, pool, id) {}

int pepp::tc::lex::SymbolDeclaration::type() const { return TYPE; }

QString pepp::tc::lex::SymbolDeclaration::type_name() const { return "Symbol"; }

pepp::tc::lex::InlineComment::InlineComment(support::LocationInterval loc, support::StringPool *pool,
                                            support::PooledString id)
    : Identifier(loc, pool, id) {}

int pepp::tc::lex::InlineComment::type() const { return TYPE; }

QString pepp::tc::lex::InlineComment::type_name() const { return "InlineComment"; }

pepp::tc::lex::Literal::Literal(support::LocationInterval loc, QString literal) : Token(loc), literal(literal) {}

int pepp::tc::lex::Literal::type() const { return TYPE; }

QString pepp::tc::lex::Literal::type_name() const { return "Literal"; }

QString pepp::tc::lex::Literal::to_string() const { return literal; }
