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
using pepp::tc::SymbolOperand;

// Simplify first, so that e.g. sym - 1 + 3 becomes the constant + symbol which is eash to match against a relocation.
SymbolOperand classify_expression(const pepp::ast::Expression &exp) {
  using Kind = SymbolOperand::Kind;
  using K = pepp::tc::expr::Kind;
  namespace expr = pepp::tc::expr;
  const auto tree = expr::simplify(exp.tree(), exp.default_type(), exp.resolve_types_of(), exp.resolve_constants_of());
  // Every symbol was a constant, so the value never moves.
  if (expr::is_constant_expression(tree)) return {};
  // Lowering referenced every identifier, including location counters, into the symbol table.
  const auto symbol = [&](expr::NodeId id) {
    const auto *identifier = std::get_if<expr::Identifier>(&tree[id]);
    const auto &name = identifier ? identifier->name : std::get<expr::LocationCounter>(tree[id]).name;
    return exp.symbol_table()->get(name).value();
  };

  static constexpr std::array symbol_only{K::Symbolic};
  static constexpr std::array constant_plus_symbol{K::Constant, K::Symbolic, K::Add};
  if (expr::matches(tree.kinds(), symbol_only)) return {Kind::Offset, symbol(0), 0};
  else if (expr::matches(tree.kinds(), constant_plus_symbol)) {
    // Sign extend addend so that ABS16 effectively wraps mod 2^16, which is important for expressions like `sym-2`.
    const i64 addend = expr::constant_values(tree, exp.default_type())[0]->as_signed();
    return {Kind::Offset, symbol(1), addend};
  }
  return {Kind::Invalid};
}

// Something which can move, such as a label or undefined symbol (e.g., not an .EQUATE).
bool relocatable(const SymbolOperand &operand) {
  return operand.kind == SymbolOperand::Kind::Offset && !(operand.symbol && operand.symbol->is_constant());
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
  if (relocatable(classify_symbol_operand(value))) {
    if (narrowing_relocation || address_bits <= constraint.bits) return true;
    return report(E(E::UnaryError::Argument_AddressDoesNotFit, to_string(constraint), line.source_interval));
  }
  // A non-constexpr call (e.g., %pcrel_hi) is defined by its relocation, which narrows it to fit.
  else if (const auto *expression = dynamic_cast<pepp::ast::Expression *>(&value);
           expression && std::ranges::any_of(expression->tree().nodes(), [](const expr::Node &node) {
             const auto *call = std::get_if<expr::Call>(&node);
             return call && call->function->category == expr::Function::Category::NonConstExpr;
           }))
    return true;
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

SymbolOperand pepp::tc::classify_symbol_operand(pepp::ast::IRValue &value) {
  using Kind = SymbolOperand::Kind;
  if (auto *symbolic = dynamic_cast<pepp::ast::Symbolic *>(&value)) return {Kind::Offset, symbolic->symbol(), 0};
  else if (auto *expression = dynamic_cast<pepp::ast::Expression *>(&value)) return classify_expression(*expression);
  return {};
}
