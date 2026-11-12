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

#include "asmb_tokens.hpp"
#include <fmt/format.h>

pepp::tc::lex::DotCommand::DotCommand(support::LocationInterval loc, std::string const *v) : Identifier(loc, v) {}

int pepp::tc::lex::DotCommand::type() const { return TYPE; }

pepp::tc::lex::MacroPlaceholder::MacroPlaceholder(support::LocationInterval loc, const std::string *v)
    : Identifier(loc, v) {}

int pepp::tc::lex::MacroPlaceholder::type() const { return TYPE; }

std::string pepp::tc::lex::MacroPlaceholder::type_name() const { return "MacroPlaceholder"; }

std::string pepp::tc::lex::MacroPlaceholder::to_string() const { return fmt::format("\\{}", view()); }

std::string pepp::tc::lex::MacroPlaceholder::repr() const { return fmt::format("{}({})", type_name(), view()); }
