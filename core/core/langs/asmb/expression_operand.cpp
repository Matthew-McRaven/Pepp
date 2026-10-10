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
#include <array>
#include "core/compile/ir_value/expression.hpp"
#include "core/compile/ir_value/numeric.hpp"
#include "core/compile/ir_value/symbolic.hpp"
#include "core/compile/ir_value/text.hpp"
#include "core/compile/lex/buffer.hpp"
#include "core/compile/lex/lexer.hpp"
#include "core/compile/symbol/entry.hpp"
#include "core/compile/symbol/leaf_table.hpp"
#include "core/compile/symbol/value.hpp"
#include "core/langs/asmb/asmb_tokens.hpp"
#include "core/langs/expr/traversal.hpp"

namespace {
namespace expr = pepp::tc::expr;

bool is_constant(const pepp::core::symbol::Entry &entry) {
  return entry.value && entry.value->type() == pepp::core::symbol::Type::Constant;
}
} // namespace

std::expected<pepp::tc::parser::ExpressionResult, pepp::tc::expr::Error>
pepp::tc::parser::parse_expression(lex::Buffer &buf, lex::ALexer &lexer, std::shared_ptr<expr::IdentifierPool> pool,
                                   const expr::Features &features, const expr::NameLocationCounter &location_counter) {
  // A rollback left an operand which was already parsed, and its text is no longer ahead of the lexer.
  if (const auto buffered = buf.buffered_tokens(); !buffered.empty()) {
    if (buffered.front()->type() == lex::ParsedExpression::TYPE) return buf.match<lex::ParsedExpression>()->operand;
    // The expression parser reads the lexer's text, which is past any buffered token. This is a bug in the assembler,
    // but is reported as an error in the source rather than ending the program.
    return std::unexpected(expr::Error{buffered.front()->location(), "Unexpected token before an expression"});
  }

  auto result = expr::parse(lexer.cursor(), std::move(pool), features, location_counter);
  if (const auto *error = std::get_if<expr::Error>(&result)) return std::unexpected(*error);
  auto *parsed = std::get_if<expr::Parsed>(&result);
  if (!parsed) return nullptr;

  // Resume lexing after the operand, which the buffer sees as a single token.
  const support::LocationInterval span{parsed->locations[parsed->tree.root()].lower(), parsed->after.location()};
  lexer.resume_at(parsed->after);
  buf.push_token(std::make_shared<lex::ParsedExpression>(span, std::make_shared<expr::Parsed>(std::move(*parsed))));
  return buf.match<lex::ParsedExpression>()->operand;
}

std::expected<std::shared_ptr<pepp::ast::IRValue>, pepp::tc::expr::Error>
pepp::tc::parser::lower(const expr::Parsed &operand, std::shared_ptr<core::symbol::LeafTable> symtab,
                        const expr::Options &options, u8 size) {
  using namespace bits;
  using K = expr::Kind;
  static constexpr std::array lone_integer{K::Integer};
  static constexpr std::array lone_character{K::Character};
  static constexpr std::array lone_identifier{K::Identifier};
  static constexpr std::array signed_integer{K::Integer, K::Plus | K::Minus};
  const auto &tree = operand.tree;
  const auto &kinds = tree.kinds();
  const auto *integer = std::get_if<expr::Integer>(&tree[0]);
  const bool decimal = integer && integer->format == expr::Integer::Format::Decimal;

  if (expr::matches(kinds, lone_integer) && decimal)
    return std::make_shared<ast::UnsignedDecimal>(integer->value, size);
  else if (expr::matches(kinds, lone_integer)) return std::make_shared<ast::Hexadecimal>(integer->value, size);
  else if (expr::matches(kinds, lone_character))
    return std::make_shared<ast::Character>(static_cast<char>(std::get<expr::Character>(tree[0]).value));
  else if (expr::matches(kinds, lone_identifier))
    return std::make_shared<ast::Symbolic>(size, symtab->reference(std::get<expr::Identifier>(tree[0]).name));
  // Only a decimal may be signed. -0x10 is an expression.
  else if (expr::matches(kinds, signed_integer) && decimal && kinds[1] == K::Minus)
    return std::make_shared<ast::SignedDecimal>(-static_cast<i64>(integer->value), size);
  else if (expr::matches(kinds, signed_integer) && decimal)
    return std::make_shared<ast::UnsignedDecimal>(integer->value, size);

  const auto reference = [&](const expr::Identifier &id) { (void)symtab->reference(id.name); };
  expr::for_each_node<expr::Identifier>(tree, reference);
  auto value = std::make_shared<ast::Expression>(tree, symtab, options, size);
  if (!value->contains_symbols())
    if (const auto evaluated = value->evaluate(); !evaluated) {
      const auto &error = evaluated.error();
      const auto root = operand.locations[tree.root()];
      return std::unexpected(expr::Error{error.node ? operand.locations[*error.node] : root, error.message});
    }
  return value;
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
