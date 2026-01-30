#include "./pep_tokens.hpp"
#include <fmt/format.h>

pepp::tc::lex::DotCommand::DotCommand(support::LocationInterval loc, std::string const *v) : Identifier(loc, v) {}

int pepp::tc::lex::DotCommand::type() const { return TYPE; }

pepp::tc::lex::MacroInvocation::MacroInvocation(support::LocationInterval loc, std::string const *v)
    : Identifier(loc, v) {}

int pepp::tc::lex::MacroInvocation::type() const { return TYPE; }

pepp::tc::lex::CharacterConstant::CharacterConstant(support::LocationInterval loc, std::string value)
    : Token(loc), value(value) {}

int pepp::tc::lex::CharacterConstant::type() const { return TYPE; }

std::string pepp::tc::lex::CharacterConstant::type_name() const { return "CharacterConstant"; }

std::string pepp::tc::lex::CharacterConstant::to_string() const { return fmt::format("'{}'", value); }

std::string pepp::tc::lex::CharacterConstant::repr() const { return fmt::format("{}({})", type_name(), value); }

pepp::tc::lex::StringConstant::StringConstant(support::LocationInterval loc, std::string const *v)
    : Identifier(loc, v) {}

int pepp::tc::lex::StringConstant::type() const { return TYPE; }

std::string pepp::tc::lex::StringConstant::type_name() const { return "StringConstant"; }

std::string pepp::tc::lex::StringConstant::to_string() const { return fmt::format("\"%1\"", view()); }

std::string pepp::tc::lex::StringConstant::repr() const { return fmt::format("%1(%2)", type_name(), view()); }
