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
#include <vector>
#include "core/compile/abstract_value/base.hpp"

namespace pepp::ast {

struct String : public BaseValue {
public:
  explicit String();
  String(std::string value);
  String(const String &other);
  String(String &&other) noexcept;
  String &operator=(String other);
  friend void swap(String &first, String &second) {
    using std::swap;
    swap(first._size, second._size);
    swap(first._bytes, second._bytes);
  }

  inline u64 serialized_size() const noexcept override { return _size; }
  [[nodiscard]] u32 serialize(bits::span<u8> dest, bits::Order destEndian = bits::Order::BigEndian,
                              u32 max_size = (u32)-1) const noexcept override;
  std::string string() const override;
  std::string raw_string() const override;

private:
  u8 _size = 0;
  std::vector<char> _bytes = {};
};

class Character : public BaseValue {
public:
  explicit Character();
  explicit Character(char value);
  Character(const Character &other);
  Character(Character &&other) noexcept;
  Character &operator=(Character other);
  friend void swap(Character &first, Character &second) {
    using std::swap;
    swap(first._value, second._value);
  }

  inline u64 serialized_size() const noexcept override { return 1; }
  [[nodiscard]] u32 serialize(bits::span<u8> dest, bits::Order destEndian = bits::Order::BigEndian,
                              u32 max_size = (u32)-1) const noexcept override;
  std::string string() const override;
  std::string raw_string() const override;

private:
  char _value = {};
};
} // namespace pepp::ast
