#include "./tokens.hpp"
#include <fmt/format.h>
#include "core/libs/bitmanip/strings.hpp"

pepp::tc::lex::Token::Token(support::LocationInterval loc) : _loc(loc) {}
pepp::tc::lex::Token::~Token() {};

std::string pepp::tc::lex::Token::to_string() const { return ""; }

std::string pepp::tc::lex::Token::repr() const { return fmt::format("{}({})", type_name(), to_string()); }

bool pepp::tc::lex::Token::mask(int mask) const { return type() & mask; }

pepp::tc::support::LocationInterval pepp::tc::lex::Token::location() const { return _loc; }

pepp::tc::support::Location pepp::tc::lex::Token::start() const { return _loc.lower(); }

pepp::tc::support::Location pepp::tc::lex::Token::end() const { return _loc.upper(); }

pepp::tc::lex::Invalid::Invalid(support::LocationInterval loc) : Token(loc) {}

int pepp::tc::lex::Invalid::type() const { return TYPE; }

std::string pepp::tc::lex::Invalid::type_name() const { return "Invalid"; }

pepp::tc::lex::EoF::EoF(support::LocationInterval loc) : Token(loc) {}

int pepp::tc::lex::EoF::type() const { return TYPE; }

std::string pepp::tc::lex::EoF::type_name() const { return "EoF"; }

pepp::tc::lex::Empty::Empty(support::LocationInterval loc) : Token(loc) {}

int pepp::tc::lex::Empty::type() const { return TYPE; }

std::string pepp::tc::lex::Empty::type_name() const { return "Empty"; }

std::string pepp::tc::lex::Empty::to_string() const { return "\n"; }

std::string pepp::tc::lex::Empty::repr() const { return fmt::format("{}()", type_name()); }

pepp::tc::lex::Integer::Integer(support::LocationInterval loc, uint64_t val, Format fmt)
    : Token(loc), format(fmt), value(val) {}

int pepp::tc::lex::Integer::type() const { return TYPE; }

std::string pepp::tc::lex::Integer::type_name() const { return "Integer"; }

std::string pepp::tc::lex::Integer::to_string() const {
  switch (format) {
  case Format::SignedDec: return fmt::format("{}", (int64_t)value);
  case Format::UnsignedDec: return fmt::format("{}", value);
  case Format::Hex: return "0x" + fmt::format("{:X}", value, 16);
  case Format::Bin: return "0b" + fmt::format("{:b}", value, 16); ;
  default: throw std::logic_error("Unrecognized Integer::Format");
  }
}

pepp::tc::lex::Identifier::Identifier(support::LocationInterval loc, std::string const *v) : Token(loc), value(v) {}

int pepp::tc::lex::Identifier::type() const { return TYPE; }

std::string_view pepp::tc::lex::Identifier::view() const { return *value; }

std::string pepp::tc::lex::Identifier::type_name() const { return "Identifier"; }

std::string pepp::tc::lex::Identifier::to_string() const { return std::string{view()}; }

pepp::tc::lex::SymbolDeclaration::SymbolDeclaration(support::LocationInterval loc, std::string const *v)
    : Identifier(loc, v) {}

int pepp::tc::lex::SymbolDeclaration::type() const { return TYPE; }

std::string pepp::tc::lex::SymbolDeclaration::type_name() const { return "Symbol"; }

pepp::tc::lex::InlineComment::InlineComment(support::LocationInterval loc, std::string const *v) : Identifier(loc, v) {}

int pepp::tc::lex::InlineComment::type() const { return TYPE; }

std::string pepp::tc::lex::InlineComment::type_name() const { return "InlineComment"; }

pepp::tc::lex::Literal::Literal(support::LocationInterval loc, std::string literal) : Token(loc), literal(literal) {}

int pepp::tc::lex::Literal::type() const { return TYPE; }

std::string pepp::tc::lex::Literal::type_name() const { return "Literal"; }

std::string pepp::tc::lex::Literal::to_string() const { return literal; }
