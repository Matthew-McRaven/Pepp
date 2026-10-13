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
#include "core/langs/asmb/codegen.hpp"
#include <array>
#include "core/compile/ir_value/expression.hpp"
#include "core/compile/ir_value/numeric.hpp"
#include "core/compile/ir_value/symbolic.hpp"
#include "core/compile/symbol/leaf_table.hpp"
#include "core/compile/symbol/value.hpp"
#include "core/langs/asmb/codegen_error.hpp"
#include "core/langs/expr/evaluator.hpp"
#include "core/langs/expr/ir.hpp"

namespace {
using pepp::tc::SymbolOffset;

namespace expr = pepp::tc::expr;

// Simplify first, so that e.g. sym - 1 + 3 becomes the constant + symbol which is eash to match against a relocation.
expr::Tree simplified(const pepp::ast::Expression &exp) {
  return expr::simplify(exp.tree(), exp.default_type(), exp.resolve_types_of(), exp.resolve_constants_of());
}

// Match a simplified expression tree against a relocation pattern. If the tree matches, return the symbol and addend.
std::optional<SymbolOffset> match(const expr::Tree &tree, const pepp::ast::Expression &exp,
                                   const pepp::tc::OperandPattern &pattern) {
  if (!expr::matches(tree.kinds(), pattern.kinds)) return std::nullopt;
  if (!pattern.function.empty()) {
    const auto *call = std::get_if<expr::Call>(&tree[tree.root()]);
    if (!call || call->function->name != pattern.function) return std::nullopt;
  }
  // Lowering referenced every identifier, including location counters, into the symbol table.
  const auto *identifier = std::get_if<expr::Identifier>(&tree[pattern.symbol]);
  const auto &name = identifier ? identifier->name : std::get<expr::LocationCounter>(tree[pattern.symbol]).name;
  // Sign extend addend so that ABS16 effectively wraps mod 2^16, which is important for expressions like `sym-2`.
  const i64 addend =
      pattern.addend ? expr::constant_values(tree, exp.default_type())[*pattern.addend]->as_signed() : 0;
  return SymbolOffset{exp.symbol_table()->get(name).value(), addend};
}

// The value an argument encodes, typed so that its sign can be checked against a constraint.
std::expected<pepp::tc::expr::Value, std::string> typed_value(const pepp::ast::IRValue &value) {
  using pepp::tc::expr::Value;
  using enum pepp::tc::expr::Signedness;
  if (const auto *expression = dynamic_cast<const pepp::ast::Expression *>(&value)) {
    if (auto evaluated = expression->evaluate(); evaluated) return *evaluated;
    else return std::unexpected(evaluated.error().message());
  } else if (const auto *number = dynamic_cast<const pepp::ast::SignedDecimal *>(&value))
    return Value{number->value(), {64, Signed}};
  else if (const auto *number = dynamic_cast<const pepp::ast::Numeric *>(&value))
    return Value{number->value(), {64, Bits}};
  else if (const auto *symbolic = dynamic_cast<const pepp::ast::Symbolic *>(&value)) {
    const auto &entry = *symbolic->symbol();
    return Value{entry.value ? entry.value->value()() : 0, {64, Bits}};
  }
  return Value{value.value_as<u64>(), {64, Bits}};
}
} // namespace

bool pepp::tc::validate_argument(DiagnosticTable &diag, const LinearIR &line, pepp::ast::IRValue &value,
                                 ArgumentConstraint constraint, u8 address_bits, bool narrowing_relocation) {
  using E = CodegenError;
  const auto report = [&](const E &error) { return diag.add_message(error.loc, error.what()), false; };
  // A symbol which can move is address-sized, whatever its current value.
  if (const auto offset = symbol_offset(value); offset && !offset->symbol->is_constant()) {
    if (narrowing_relocation || address_bits <= constraint.bits) return true;
    return report(E(E::UnaryError::Argument_AddressDoesNotFit, to_string(constraint), line.source_interval));
  }
  const auto typed = typed_value(value);
  if (!typed) return report(E(E::UnaryError::Expression_Invalid, typed.error(), line.source_interval));
  return validate_value(diag, line, *typed, constraint);
}

bool pepp::tc::validate_value(DiagnosticTable &diag, const LinearIR &line, expr::Value value,
                              ArgumentConstraint constraint) {
  using E = CodegenError;
  const auto report = [&](const E &error) { return diag.add_message(error.loc, error.what()), false; };
  if (!aligned(value, constraint)) return report(E(E::NullaryError::Argument_OddOffset, line.source_interval));
  else if (!satisfies(value, constraint))
    return report(E(E::UnaryError::Argument_ValueDoesNotFit, to_string(constraint), line.source_interval));
  return true;
}

bool pepp::tc::value_needs_relocation(pepp::ast::IRValue &value) {
  if (const auto *symbolic = dynamic_cast<pepp::ast::Symbolic *>(&value)) return !symbolic->symbol()->is_constant();
  // Simplification replaces constant symbols with their values, so all remaining symbols need relocation.
  else if (const auto *expression = dynamic_cast<pepp::ast::Expression *>(&value))
    return !expr::is_constant_expression(simplified(*expression));
  return false;
}

std::optional<SymbolOffset> pepp::tc::symbol_offset(pepp::ast::IRValue &value) {
  for (const auto &pattern : symbol_offset_patterns)
    if (auto offset = match_symbol_operand(value, pattern)) return offset;
  return std::nullopt;
}

std::optional<SymbolOffset> pepp::tc::match_symbol_operand(pepp::ast::IRValue &value, const OperandPattern &pattern) {
  using K = expr::Kind;
  static constexpr std::array lone_symbol{K::Identifier};
  if (auto *symbolic = dynamic_cast<pepp::ast::Symbolic *>(&value)) {
    if (!pattern.function.empty() || !expr::matches(lone_symbol, pattern.kinds)) return std::nullopt;
    return SymbolOffset{symbolic->symbol(), 0};
  } else if (const auto *expression = dynamic_cast<pepp::ast::Expression *>(&value))
    return match(simplified(*expression), *expression, pattern);
  return std::nullopt;
}
