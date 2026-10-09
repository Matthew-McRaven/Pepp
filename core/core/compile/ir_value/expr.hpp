/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once
#include "core/compile/ir_value/base.hpp"
#include "core/compile/ir_value/numeric.hpp"
#include "core/compile/ir_value/symbolic.hpp"

namespace pepp::ast {
struct InfixExpression : public IRValue {
public:
  enum class Op : u8 {
    Nil, // Do not evaluate lhs or rhs; return 0.
    Addition,
    Subtraction,
  };
  explicit InfixExpression() noexcept = default;
  InfixExpression(Op op, std::shared_ptr<IRValue> lhs, std::shared_ptr<IRValue> rhs, u8 size);
  friend void swap(InfixExpression &first, InfixExpression &second) noexcept {
    using std::swap;
    swap(first._op, second._op);
    swap(first._size, second._size);
    swap(first._lhs, second._lhs);
    swap(first._rhs, second._rhs);
  }

  Op op() const noexcept { return _op; }
  const std::shared_ptr<IRValue> &lhs() const noexcept { return _lhs; }
  const std::shared_ptr<IRValue> &rhs() const noexcept { return _rhs; }

  u64 serialized_size() const noexcept override;
  u64 minimum_size() const noexcept override;
  [[nodiscard]] u32 serialize(bits::span<u8> dest, bits::Order destEndian = bits::Order::BigEndian,
                              u32 max_size = (u32)-1) const noexcept override;
  std::string string() const override;
  std::string raw_string() const override;

protected:
  u64 evaluate() const noexcept;
  Op _op = Op::Nil;
  u8 _size = 0;
  std::shared_ptr<IRValue> _lhs = nullptr, _rhs = nullptr;
};

// True if value is, or an infix expression containing, a Symbolic, or is an Expression which names a symbol.
bool contains_symbol(const IRValue &value) noexcept;
} // namespace pepp::ast
