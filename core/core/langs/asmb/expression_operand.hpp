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
#include <optional>
#include "core/compile/ir_value/base.hpp"
#include "core/compile/source/location.hpp"
#include "core/langs/expr/error.hpp"
#include "core/langs/expr/evaluator.hpp"
#include "core/langs/expr/parser.hpp"

namespace pepp::tc::lex {
struct ALexer;
class Buffer;
} // namespace pepp::tc::lex
namespace pepp::core::symbol {
class LeafTable;
}

// Expression operands for the assemblers, which share expression syntax but not their parsers.
namespace pepp::tc::parser {

// An operand parsed as an expression, but not yet given a meaning. lower() converts it to an IRValue for use in the
// assembler.
using ExpressionResult = std::shared_ptr<const expr::Parsed>;

// Enter the subexpression parser at buf's next token. Returns nullptr if no expression starts there. Otherwise the
// operand becomes one ParsedExpression token in buf, so a rollback over it replays the operand rather than parsing
// again. No symbols are referenced.
std::expected<ExpressionResult, expr::Error> parse_expression(lex::Buffer &buf, lex::ALexer &lexer,
                                                              std::shared_ptr<expr::IdentifierPool> pool);

// Convert a parsed expression to an IRValue, prefering the most specfic IRValue possible. Only if no specific pattern
// matches is an Expression IRValue created. Identifiers are referenced into the symbol table at this time. Symbol-free
// expressions that fail to evaluate raise an error at this time.
std::expected<std::shared_ptr<ast::IRValue>, expr::Error> lower(const expr::Parsed &operand,
                                                                std::shared_ptr<core::symbol::LeafTable> symtab,
                                                                const expr::Options &options, u8 size);

// An .EQUATE's value, or nullopt if the expression cannot be constant-evaluated.
// consteval could fail because a symbolic argument refers to a program location, or because the symbol has not been
// defined (e.g., a later equate).
std::expected<std::optional<u64>, expr::Error> equate_value(ast::IRValue &arg, support::LocationInterval location);

} // namespace pepp::tc::parser
