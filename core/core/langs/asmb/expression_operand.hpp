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

// Enter the subexpression parser starting at buf's next token. Returns a nullptr if an expression cannot be constructed
// or if the expression is an atom (a lone integer, character, or symbol, signed decimal). Otherwise its identifiers are
// referenced in symtab, and the expression becomes one ParsedExpression token in buf, allowing a rollback over this
// token to skip the nested parser in the future. A symbol-free expression which cannot be evaluated is an error. buf's
// unmatched tokens are first returned to lexer, so no currently-live Checkpoint may be past buf's head.
std::expected<std::shared_ptr<ast::IRValue>, expr::Error>
expression_operand(lex::Buffer &buf, lex::ALexer &lexer, std::shared_ptr<expr::IdentifierPool> pool,
                   std::shared_ptr<core::symbol::LeafTable> symtab, const expr::Options &options, u8 size);

// An .EQUATE's value, or nullopt if the expression cannot be constant-evaluated.
// consteval could fail because a symbolic argument refers to a program location, or because the symbol has not been
// defined (e.g., a later equate).
std::expected<std::optional<u64>, expr::Error> equate_value(ast::IRValue &arg, support::LocationInterval location);

} // namespace pepp::tc::parser
