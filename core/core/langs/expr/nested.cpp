/*
 * Copyright (c) 2026. Stanley Warford, Matthew McRaven
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "core/langs/expr/nested.hpp"
#include "core/compile/lex/buffer.hpp"
#include "core/compile/lex/lexer.hpp"

pepp::tc::lex::ParsedExpression::ParsedExpression(support::LocationInterval loc,
                                                  std::shared_ptr<const pepp::tc::expr::Parsed> operand)
    : Token(loc), operand(std::move(operand)) {}

int pepp::tc::lex::ParsedExpression::type() const { return TYPE; }

std::string pepp::tc::lex::ParsedExpression::type_name() const { return "ParsedExpression"; }

std::string pepp::tc::lex::ParsedExpression::to_string() const { return pepp::tc::expr::to_infix(operand->tree); }

std::expected<std::shared_ptr<const pepp::tc::expr::Parsed>, pepp::tc::expr::Error>
pepp::tc::expr::parse_nested(lex::Buffer &buf, lex::ALexer &lexer, std::shared_ptr<IdentifierPool> pool,
                             const Options &options, const NameLocationCounter &location_counter,
                             const ValueOf &constant_of) {
  // The expression parser reads the lexer's text, which is past any buffered token. This is a bug in the caller, but
  // is reported as an error in the source rather than ending the program.
  if (const auto buffered = buf.buffered_tokens(); !buffered.empty())
    return std::unexpected(Error{buffered.front()->location(), NullaryError::Syntax_BufferedToken});

  auto result = parse(lexer.cursor(), std::move(pool), options, location_counter, constant_of);
  if (const auto *error = std::get_if<Error>(&result)) return std::unexpected(*error);
  auto *parsed = std::get_if<Parsed>(&result);
  if (!parsed) return nullptr;

  // Resume lexing after the expression, which the buffer sees as a single token.
  const support::LocationInterval span{parsed->locations[parsed->tree.root()].lower(), parsed->after.location()};
  lexer.resume_at(parsed->after);
  buf.push_token(std::make_shared<lex::ParsedExpression>(span, std::make_shared<Parsed>(std::move(*parsed))));
  return buf.match<lex::ParsedExpression>()->operand;
}
