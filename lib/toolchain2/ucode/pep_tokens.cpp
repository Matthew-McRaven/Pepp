#include "pep_tokens.hpp"
#include <fmt/format.h>
pepp::tc::lex::LineNumber::LineNumber(support::LocationInterval loc, int lineNo) : Token(loc), line(lineNo) {}

int pepp::tc::lex::LineNumber::type() const { return TYPE; }

std::string pepp::tc::lex::LineNumber::type_name() const { return "LineNumber"; }

std::string pepp::tc::lex::LineNumber::to_string() const { return fmt::format("{}.", line); }

std::string pepp::tc::lex::LineNumber::repr() const { return fmt::format("{}({})", type_name(), line); }

pepp::tc::lex::UnitPre::UnitPre(support::LocationInterval loc) : Token(loc) {}

int pepp::tc::lex::UnitPre::type() const { return TYPE; }

std::string pepp::tc::lex::UnitPre::type_name() const { return "UnitPre"; }

std::string pepp::tc::lex::UnitPre::to_string() const { return fmt::format("{}:", type_name()); }

std::string pepp::tc::lex::UnitPre::repr() const { return fmt::format("{}()", type_name()); }

pepp::tc::lex::UnitPost::UnitPost(support::LocationInterval loc) : Token(loc) {}

int pepp::tc::lex::UnitPost::type() const { return TYPE; }

std::string pepp::tc::lex::UnitPost::type_name() const { return "UnitPost"; }

std::string pepp::tc::lex::UnitPost::to_string() const { return fmt::format("{}:", type_name()); }

std::string pepp::tc::lex::UnitPost::repr() const { return fmt::format("{}()", type_name()); }
