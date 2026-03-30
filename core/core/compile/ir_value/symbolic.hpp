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
#include <memory>
#include "core/compile/ir_value/base.hpp"

namespace pepp::core::symbol {
class Entry;
}
namespace pepp::ast {

// Represents a value that propogates the value of another symbol within the current table.
// This value cannot be relocated, since it acts like a numeric constant rather than a location.
// This class provides no API for detecting or prevent binding loops.
struct Symbolic : public IRValue {
public:
  explicit Symbolic();
  Symbolic(u8 ptr_size_bytes, std::shared_ptr<core::symbol::Entry> value);
  Symbolic(const Symbolic &other);
  Symbolic(Symbolic &&other) noexcept;
  Symbolic &operator=(Symbolic other);
  friend void swap(Symbolic &first, Symbolic &second) noexcept {
    using std::swap;
    swap(first._ptr_size_bytes, second._ptr_size_bytes);
    swap(first._value, second._value);
  }
  std::shared_ptr<core::symbol::Entry> symbol();
  std::shared_ptr<const core::symbol::Entry> symbol() const;
  inline u64 serialized_size() const noexcept override { return _ptr_size_bytes; }
  u64 minimum_size() const noexcept override;
  [[nodiscard]] u32 serialize(bits::span<u8> dest, bits::Order destEndian = bits::Order::BigEndian,
                              u32 max_size = (u32)-1) const noexcept override;

  std::string string() const override;
  std::string raw_string() const override;

private:
  u16 _ptr_size_bytes;
  std::shared_ptr<core::symbol::Entry> _value = nullptr;
};
} // namespace pepp::ast
