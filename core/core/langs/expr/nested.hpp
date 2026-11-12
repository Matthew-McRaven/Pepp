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
#pragma once
#include <expected>
#include <memory>
#include "core/compile/lex/tokens.hpp"
#include "core/langs/expr/error.hpp"
#include "core/langs/expr/parser.hpp"

namespace pepp::tc::lex {
struct ALexer;
class Buffer;

// An expression parsed by a nested parser (e.g., the expression parser)
struct ParsedExpression : public Token {
  ParsedExpression(support::LocationInterval loc, std::shared_ptr<const pepp::tc::expr::Parsed> operand);
  static constexpr int TYPE = static_cast<int>(CommonTokenType::ParsedExpression);
  int type() const override;
  std::string type_name() const override;
  std::string to_string() const override;

  std::shared_ptr<const pepp::tc::expr::Parsed> operand;
};
} // namespace pepp::tc::lex

// Parsing an expression from inside another language's parser, as the assemblers and linker scripts do.
namespace pepp::tc::expr {
// Enter the expression parser at buf's next token. Returns nullptr if no expression starts there. Otherwise the
// expression becomes one ParsedExpression token in buf, standing in for its text. It is an error if any token is
// already buffered, since the expression parser reads the lexer's text from after it. options, location_counter, and
// constant_of are as for parse.
std::expected<std::shared_ptr<const Parsed>, Error>
parse_nested(lex::Buffer &buf, lex::ALexer &lexer, std::shared_ptr<IdentifierPool> pool, const Options &options = {},
             const NameLocationCounter &location_counter = {}, const ValueOf &constant_of = {});
} // namespace pepp::tc::expr
