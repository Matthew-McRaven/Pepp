/*
 * Copyright (c) 2025-2026 J. Stanley Warford, Matthew McRaven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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
