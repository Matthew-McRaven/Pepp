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
#include "core/compile/ir_value/symbolic.hpp"
#include "core/compile/symbol/leaf_table.hpp"
#include "core/langs/expr/evaluator.hpp"
#include "core/langs/expr/ir.hpp"

namespace {
using pepp::tc::SymbolOperand;

// Simplify first, so that e.g. sym - 1 + 3 becomes the constant + symbol which is eash to match against a relocation.
SymbolOperand classify_expression(const pepp::ast::Expression &exp) {
  using Kind = SymbolOperand::Kind;
  using K = pepp::tc::expr::Kind;
  namespace expr = pepp::tc::expr;
  const auto tree = expr::simplify(exp.tree(), exp.options(), exp.resolve_types_of(), exp.resolve_constants_of());
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
    const i64 addend = expr::constant_values(tree, exp.options())[0]->as_signed();
    return {Kind::Offset, symbol(1), addend};
  }
  return {Kind::Invalid};
}

} // namespace

SymbolOperand pepp::tc::classify_symbol_operand(pepp::ast::IRValue &value) {
  using Kind = SymbolOperand::Kind;
  if (auto *symbolic = dynamic_cast<pepp::ast::Symbolic *>(&value)) return {Kind::Offset, symbolic->symbol(), 0};
  else if (auto *expression = dynamic_cast<pepp::ast::Expression *>(&value)) return classify_expression(*expression);
  return {};
}
