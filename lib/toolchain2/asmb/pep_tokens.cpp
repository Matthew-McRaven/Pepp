#include "./pep_tokens.hpp"

pepp::tc::lex::DotCommand::DotCommand(support::LocationInterval loc, QString const *v) : Identifier(loc, v) {}

int pepp::tc::lex::DotCommand::type() const { return TYPE; }

pepp::tc::lex::MacroInvocation::MacroInvocation(support::LocationInterval loc, QString const *v) : Identifier(loc, v) {}

int pepp::tc::lex::MacroInvocation::type() const { return TYPE; }

pepp::tc::lex::CharacterConstant::CharacterConstant(support::LocationInterval loc, QString value)
    : Token(loc), value(value) {}

int pepp::tc::lex::CharacterConstant::type() const { return TYPE; }

QString pepp::tc::lex::CharacterConstant::type_name() const { return "CharacterConstant"; }

QString pepp::tc::lex::CharacterConstant::to_string() const { return QStringLiteral("'%1'").arg(value); }

QString pepp::tc::lex::CharacterConstant::repr() const { return QStringLiteral("%1(%2)").arg(type_name(), value); }

pepp::tc::lex::StringConstant::StringConstant(support::LocationInterval loc, QString const *v) : Identifier(loc, v) {}

int pepp::tc::lex::StringConstant::type() const { return TYPE; }

QString pepp::tc::lex::StringConstant::type_name() const { return "StringConstant"; }

QString pepp::tc::lex::StringConstant::to_string() const { return QStringLiteral("\"%1\"").arg(view()); }

QString pepp::tc::lex::StringConstant::repr() const { return QStringLiteral("%1(%2)").arg(type_name()).arg(view()); }
