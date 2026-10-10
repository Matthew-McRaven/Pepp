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
#include "core/compile/ir_value/base.hpp"
#include "core/langs/expr/error.hpp"
#include "core/langs/expr/evaluator.hpp"
#include "core/langs/expr/ir.hpp"

namespace pepp::core::symbol {
class LeafTable;
}

namespace pepp::ast {

// An expression parsed by tc::expr.
class Expression : public IRValue {
public:
  // Identifiers are looked up in symtab, which may be null if the tree names no symbols.
  // `size` parameter is the default size of the expression in bytes for when evaluation fails.
  Expression(tc::expr::Tree tree, std::shared_ptr<const core::symbol::LeafTable> symtab, tc::expr::Options options,
             u8 size);

  const tc::expr::Tree &tree() const noexcept { return _tree; }
  const tc::expr::Options &options() const noexcept { return _options; }
  const std::shared_ptr<const core::symbol::LeafTable> &symbol_table() const noexcept { return _symtab; }
  bool contains_symbols() const noexcept;

  // Return a function which returns values of any defined symbol which is a constant and nullopt otherwise.
  tc::expr::ValueOf resolve_constants_of() const;
  // Return a function which returns the values of any defined symbol and nullopt for undefined symbols.
  tc::expr::ValueOf resolve_values_of() const;
  // Return a function which returns the type of any symbol, as resolve_values_of() would type its value.
  tc::expr::TypeOf resolve_types_of() const;

  // Evaluate with resolve_values_of().
  std::expected<tc::expr::Value, tc::expr::EvaluationError> evaluate() const;

  u64 serialized_size() const noexcept override { return _size; }
  // The fewest bytes which hold the value as either signed or unsigned. If evaluate() fails, returns serialized_size().
  u64 minimum_size() const noexcept override;
  // Writes 0 if the expression cannot be evaluate()'ed.
  [[nodiscard]] u32 serialize(bits::span<u8> dest, bits::Order destEndian = bits::Order::BigEndian,
                              u32 max_size = (u32)-1) const noexcept override;
  std::string string() const override;
  std::string raw_string() const override;

private:
  tc::expr::Tree _tree;
  std::shared_ptr<const core::symbol::LeafTable> _symtab;
  tc::expr::Options _options;
  u8 _size = 0;
};

// True if value is a Symbolic, or an Expression which names a symbol.
bool contains_symbol(const IRValue &value) noexcept;
} // namespace pepp::ast
