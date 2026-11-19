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
#include <algorithm>
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
  const auto report = [&](const E &error) { return diag.add_message(error.loc, error.what(), Severity::Error), false; };
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
  const auto report = [&](const E &error) { return diag.add_message(error.loc, error.what(), Severity::Error), false; };
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

bool pepp::tc::SimplifiedOperand::matches(const OperandPattern &pattern) const {
  static constexpr std::array lone_symbol{expr::Kind::Identifier};
  // If empty, then it must be a lone symbol.
  if (tree.empty()) return pattern.function.empty() && expr::matches(lone_symbol, pattern.kinds);
  else if (!expr::matches(tree.kinds(), pattern.kinds)) return false;
  // If not a function call, then matching on types of tree nodes is sufficient.
  else if (pattern.function.empty()) return true;
  // Otherwise, check that the root node contains the expected function.
  const auto *call = std::get_if<expr::Call>(&tree[tree.root()]);
  return call && call->function->name == pattern.function;
}

std::optional<pepp::tc::SimplifiedOperand> pepp::tc::simplify_operand(pepp::ast::IRValue &value) {
  using namespace bits;
  if (auto *symbolic = dynamic_cast<pepp::ast::Symbolic *>(&value))
    return SimplifiedOperand{.symbols = {symbolic->symbol()}};
  const auto *expression = dynamic_cast<pepp::ast::Expression *>(&value);
  if (!expression) return std::nullopt;

  SimplifiedOperand ret{.tree = simplified(*expression)};
  const auto &kinds = ret.tree.kinds();
  auto symbol = ret.symbols.begin();
  std::optional<std::size_t> constant;
  for (std::size_t it = 0; it < kinds.size(); it++) {
    if (any(kinds[it] & expr::Kind::Constant) && !constant) constant = it;
    else if (any(kinds[it] & expr::Kind::Symbolic) && symbol != ret.symbols.end()) {
      // Lowering referenced every identifier, including location counters, into the symbol table.
      const auto &node = ret.tree[static_cast<expr::NodeId>(it)];
      const auto *identifier = std::get_if<expr::Identifier>(&node);
      const auto &name = identifier ? identifier->name : std::get<expr::LocationCounter>(node).name;
      *symbol++ = expression->symbol_table()->get(name).value();
    }
  }
  // Sign extend addend so that ABS16 effectively wraps mod 2^16, which is important for expressions like `sym-2`.
  if (constant) ret.addend = expr::constant_values(ret.tree, expression->default_type())[*constant]->as_signed();
  return ret;
}

std::optional<SymbolOffset> pepp::tc::symbol_offset(pepp::ast::IRValue &value) {
  const auto operand = simplify_operand(value);
  if (operand && (operand->matches(symbol_pattern) || operand->matches(constant_plus_symbol_pattern)))
    return operand->target();
  return std::nullopt;
}

void pepp::tc::reject_absolute_placement(DiagnosticTable &diag, const IRProgram &prog) {
  using E = CodegenError;
  for (const auto &line : prog) {
    if (line->type() != DotOrg::TYPE) continue;
    const bool burn = static_cast<const DotOrg &>(*line).behavior == DotOrg::Behavior::BURN;
    const E error(E::UnaryError::Directive_NotRelocatable, burn ? ".BURN" : ".ORG", line->source_interval);
    diag.add_message(error.loc, error.what(), Severity::Fatal);
  }
}
