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
#include "core/langs/asmb/expression_operand.hpp"
#include "core/compile/ir_value/expression.hpp"
#include "core/compile/ir_value/symbolic.hpp"
#include "core/compile/lex/buffer.hpp"
#include "core/compile/lex/lexer.hpp"
#include "core/compile/symbol/entry.hpp"
#include "core/compile/symbol/leaf_table.hpp"
#include "core/compile/symbol/value.hpp"
#include "core/langs/asmb/asmb_tokens.hpp"
#include "core/langs/expr/traversal.hpp"

namespace {
namespace expr = pepp::tc::expr;

bool is_atom(const expr::Parsed &parsed) {
  using K = expr::Kind;
  const auto &kinds = parsed.tree.kinds();
  if (kinds.size() == 1) return true;
  else if (kinds.size() != 2 || (kinds[1] != K::Plus && kinds[1] != K::Minus)) return false;
  // The lexer only combines a sign into a decimal without spacing, so -0x10 and - 3 are expressions.
  const auto *integer = std::get_if<expr::Integer>(&parsed.tree[0]);
  const bool attached = parsed.locations[1].lower().column + 1 == parsed.locations[0].lower().column;
  return integer && integer->format == expr::Integer::Format::Decimal && attached;
}

bool is_constant(const pepp::core::symbol::Entry &entry) {
  return entry.value && entry.value->type() == pepp::core::symbol::Type::Constant;
}
} // namespace

std::expected<std::shared_ptr<pepp::ast::IRValue>, pepp::tc::expr::Error>
pepp::tc::parser::expression_operand(lex::Buffer &buf, lex::ALexer &lexer, std::shared_ptr<expr::IdentifierPool> pool,
                                     std::shared_ptr<core::symbol::LeafTable> symtab, const expr::Options &options,
                                     u8 size) {
  const auto reference = [&](const expr::Identifier &id) { (void)symtab->reference(id.name); };
  // A rollback left an expression which was already parsed, and its text is no longer ahead of the lexer. Reference its
  // symbols again, since abandoning an alternative may have removed them.
  if (const auto buffered = buf.buffered_tokens();
      !buffered.empty() && buffered.front()->type() == lex::ParsedExpression::TYPE) {
    auto value = buf.match<lex::ParsedExpression>()->value;
    // Ensure that all identifiers are entered in the symbol table, because previous references may have been dropped.
    if (const auto expression = std::dynamic_pointer_cast<ast::Expression>(value))
      expr::for_each_node<expr::Identifier>(expression->tree(), reference);
    return value;
  }
  // Sub-parser can't consume our buffered tokens (since they may be of different types).
  // Discard uncommited tokens so we can pick a different grammar-level alternative before.
  // e.g., prevent a keyword (literal) from being lexed as an expression in a different context.
  buf.unbuffer();

  // Temporarily delegate parsing expression sub-parser.
  const auto start = lexer.cursor();
  const auto result = expr::parse(start, std::move(pool));
  if (const auto *error = std::get_if<expr::Error>(&result)) return std::unexpected(*error);
  const auto *parsed = std::get_if<expr::Parsed>(&result);
  // Atoms (identifiers, unsigned decimals, hex, chars) are better represented with our specialized IR values rather
  // than a generic expression.
  if (!parsed || is_atom(*parsed)) return nullptr;

  // Evaluate constant expression to report errors, such as 1/0, at parse time.
  auto value = std::make_shared<ast::Expression>(parsed->tree, symtab, options, size);
  const support::LocationInterval span{parsed->locations[parsed->tree.root()].lower(), parsed->after.location()};
  if (!value->contains_symbols())
    if (const auto evaluated = value->evaluate(); !evaluated) {
      const auto &error = evaluated.error();
      return std::unexpected(expr::Error{error.node ? parsed->locations[*error.node] : span, error.message});
    }

  // Ensure that all identifiers are registered in the symbol table.
  expr::for_each_node<expr::Identifier>(parsed->tree, reference);

  // Resume lexing after the expression, which the buffer sees as a single token.
  lexer.resume_at(parsed->after);
  buf.push_token(std::make_shared<lex::ParsedExpression>(span, value), start);
  return buf.match<lex::ParsedExpression>()->value;
}

std::expected<std::optional<u64>, pepp::tc::expr::Error>
pepp::tc::parser::equate_value(ast::IRValue &arg, support::LocationInterval location) {
  if (auto *symbolic = dynamic_cast<ast::Symbolic *>(&arg)) {
    if (!is_constant(*symbolic->symbol())) return std::nullopt;
    auto masked = symbolic->symbol()->value->value();
    return masked();
  } else if (auto *expression = dynamic_cast<ast::Expression *>(&arg)) {
    const auto &tree = expression->tree();
    const auto result = expr::evaluate_expression(tree, expression->options(), expression->resolve_constants_of());
    if (result) return result->bits;
    // Failing at a symbol means the symbol is not a constant
    else if (result.error().node && std::holds_alternative<expr::Identifier>(tree[*result.error().node]))
      return std::nullopt;
    return std::unexpected(expr::Error{location, result.error().message});
  }
  return arg.value_as<u64>();
}
