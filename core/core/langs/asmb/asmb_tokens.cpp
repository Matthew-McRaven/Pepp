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
#include "core/langs/expr/parser.hpp"

pepp::tc::lex::DotCommand::DotCommand(support::LocationInterval loc, std::string const *v) : Identifier(loc, v) {}

int pepp::tc::lex::DotCommand::type() const { return TYPE; }

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

std::string pepp::tc::lex::StringConstant::to_string() const { return fmt::format("\"{}\"", view()); }

std::string pepp::tc::lex::StringConstant::repr() const { return fmt::format("{}({})", type_name(), view()); }

pepp::tc::lex::MacroPlaceholder::MacroPlaceholder(support::LocationInterval loc, const std::string *v)
    : Identifier(loc, v) {}

int pepp::tc::lex::MacroPlaceholder::type() const { return TYPE; }

std::string pepp::tc::lex::MacroPlaceholder::type_name() const { return "MacroPlaceholder"; }

std::string pepp::tc::lex::MacroPlaceholder::to_string() const { return fmt::format("\\{}", view()); }

std::string pepp::tc::lex::MacroPlaceholder::repr() const { return fmt::format("{}({})", type_name(), view()); }

pepp::tc::lex::ParsedExpression::ParsedExpression(support::LocationInterval loc,
                                                  std::shared_ptr<const pepp::tc::expr::Parsed> operand)
    : Token(loc), operand(std::move(operand)) {}

int pepp::tc::lex::ParsedExpression::type() const { return TYPE; }

std::string pepp::tc::lex::ParsedExpression::type_name() const { return "ParsedExpression"; }

std::string pepp::tc::lex::ParsedExpression::to_string() const { return pepp::tc::expr::to_infix(operand->tree); }
