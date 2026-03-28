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

namespace pepp::ast {
struct Numeric : public IRValue {
public:
  explicit Numeric() noexcept;
  Numeric(u64 value, u8 size) noexcept;
  friend void swap(Numeric &first, Numeric &second) {
    using std::swap;
    swap(first._size, second._size);
    swap(first._value, second._value);
  }

  inline u64 serialized_size() const noexcept override { return _size; }
  [[nodiscard]] u32 serialize(bits::span<u8> dest, bits::Order destEndian = bits::Order::BigEndian,
                              u32 max_size = (u32)-1) const noexcept override;

  void set_value(u64 value) noexcept { _value = value; }

protected:
  Numeric(const Numeric &other);
  Numeric &operator=(const Numeric &other);
  u8 _size = 0;
  u64 _value = 0;
};

class SignedDecimal : public Numeric {
public:
  explicit SignedDecimal() noexcept;
  SignedDecimal(i64 value, u8 size) noexcept;
  SignedDecimal(const SignedDecimal &other) noexcept;
  SignedDecimal(SignedDecimal &&other) noexcept;
  SignedDecimal &operator=(SignedDecimal other);
  friend void swap(SignedDecimal &first, SignedDecimal &second) {
    using std::swap;
    swap((Numeric &)first, (Numeric &)second);
  }
  std::string string() const override;
  std::string raw_string() const override;
};

class UnsignedDecimal : public Numeric {
public:
  explicit UnsignedDecimal() noexcept;
  UnsignedDecimal(u64 value, u8 size) noexcept;
  UnsignedDecimal(const UnsignedDecimal &other) noexcept;
  UnsignedDecimal(UnsignedDecimal &&other) noexcept;
  UnsignedDecimal &operator=(UnsignedDecimal other);
  friend void swap(UnsignedDecimal &first, UnsignedDecimal &second) {
    using std::swap;
    swap((Numeric &)first, (Numeric &)second);
  }

  std::string string() const override;
  std::string raw_string() const override;
};

class Hexadecimal : public Numeric {
public:
  explicit Hexadecimal() noexcept;
  Hexadecimal(u64 value, u8 size) noexcept;
  Hexadecimal(const Hexadecimal &other) noexcept;
  Hexadecimal(Hexadecimal &&other) noexcept;
  Hexadecimal &operator=(Hexadecimal other);
  friend void swap(Hexadecimal &first, Hexadecimal &second) {
    using std::swap;
    swap((Numeric &)first, (Numeric &)second);
  }

  std::string string() const override;
  std::string raw_string() const override;
};
} // namespace pepp::ast
