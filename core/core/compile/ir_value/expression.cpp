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
#include "core/compile/ir_value/expression.hpp"
#include <algorithm>
#include "core/compile/symbol/entry.hpp"
#include "core/compile/symbol/leaf_table.hpp"
#include "core/compile/symbol/value.hpp"
#include "core/math/bitmanip/copy.hpp"
#include "core/math/bitmanip/log2.hpp"
#include "core/math/bitmanip/mask.hpp"

namespace {
using pepp::tc::expr::Options;
using pepp::tc::expr::Value;
using enum pepp::tc::expr::Signedness;

bool is_constant(const pepp::core::symbol::Entry &entry) {
  return entry.value && entry.value->type() == pepp::core::symbol::Type::Constant;
}

// An .EQUATE is a constant of its own width; any other symbol names an address.
Value symbol_value(const pepp::core::symbol::Entry &entry, const Options &options) {
  if (!entry.value) return {0, {options.int_bits, Unsigned}};
  auto value = entry.value->value();
  if (is_constant(entry)) {
    const u8 width = static_cast<u8>(value.byteCount * 8);
    if (width == 8 || width == 16 || width == 32 || width == 64) return {value(), {width, Bits}};
  }
  return {value() & bits::mask(options.int_bits / 8), {options.int_bits, Unsigned}};
}
} // namespace

pepp::ast::Expression::Expression(tc::expr::Tree tree, std::shared_ptr<const core::symbol::LeafTable> symtab,
                                  tc::expr::Options options, u8 size)
    : _tree(std::move(tree)), _symtab(std::move(symtab)), _options(options), _size(size) {}

bool pepp::ast::Expression::contains_symbols() const noexcept {
  return std::ranges::contains(_tree.kinds(), tc::expr::Kind::Identifier);
}

pepp::tc::expr::ValueOf pepp::ast::Expression::resolve_constants_of() const {
  auto symtab = _symtab;
  auto options = _options;
  return [symtab, options](const tc::expr::Identifier &id) -> std::optional<Value> {
    if (const auto entry = symtab ? symtab->get(id.name) : std::nullopt; entry && is_constant(**entry))
      return symbol_value(**entry, options);
    return std::nullopt;
  };
}

pepp::tc::expr::ValueOf pepp::ast::Expression::resolve_values_of() const {
  auto symtab = _symtab;
  auto options = _options;
  return [symtab, options](const tc::expr::Identifier &id) -> std::optional<Value> {
    if (const auto entry = symtab ? symtab->get(id.name) : std::nullopt) return symbol_value(**entry, options);
    return std::nullopt;
  };
}

std::expected<pepp::tc::expr::Value, pepp::tc::expr::EvaluationError> pepp::ast::Expression::evaluate() const {
  return tc::expr::evaluate_expression(_tree, _options, resolve_values_of());
}

u64 pepp::ast::Expression::minimum_size() const noexcept {
  const auto value = evaluate();
  if (!value) return _size;
  return std::min(bits::unsigned_bytecount(value->bits), bits::signed_bytecount(value->as_signed()));
}

u32 pepp::ast::Expression::serialize(bits::span<u8> dest, bits::Order destEndian, u32 max_size) const noexcept {
  const auto value = evaluate();
  const u64 v = value ? value->bits : 0;
  const auto size = std::min<u32>(max_size, _size);
  bits::memcpy_endian(dest, destEndian, bits::span<const u8>{reinterpret_cast<const u8 *>(&v), sizeof(v)},
                      bits::hostOrder());
  return size;
}

std::string pepp::ast::Expression::string() const { return tc::expr::to_infix(_tree); }

std::string pepp::ast::Expression::raw_string() const { return string(); }
