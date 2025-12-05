#include "pep_tokens.hpp"
pepp::tc::lex::LineNumber::LineNumber(support::LocationInterval loc, int lineNo) : Token(loc), line(lineNo) {}

int pepp::tc::lex::LineNumber::type() const { return TYPE; }

QString pepp::tc::lex::LineNumber::type_name() const { return "LineNumber"; }

QString pepp::tc::lex::LineNumber::to_string() const { return QStringLiteral("%1.").arg(line); }

QString pepp::tc::lex::LineNumber::repr() const { return QStringLiteral("%1(%2)").arg(type_name()).arg(line); }

pepp::tc::lex::UnitPre::UnitPre(support::LocationInterval loc) : Token(loc) {}

int pepp::tc::lex::UnitPre::type() const { return TYPE; }

QString pepp::tc::lex::UnitPre::type_name() const { return "UnitPre"; }

QString pepp::tc::lex::UnitPre::to_string() const { return QStringLiteral("%1:").arg(type_name()); }

QString pepp::tc::lex::UnitPre::repr() const { return QStringLiteral("%1()").arg(type_name()); }

pepp::tc::lex::UnitPost::UnitPost(support::LocationInterval loc) : Token(loc) {}

int pepp::tc::lex::UnitPost::type() const { return TYPE; }

QString pepp::tc::lex::UnitPost::type_name() const { return "UnitPost"; }

QString pepp::tc::lex::UnitPost::to_string() const { return QStringLiteral("%1:").arg(type_name()); }

QString pepp::tc::lex::UnitPost::repr() const { return QStringLiteral("%1()").arg(type_name()); }
