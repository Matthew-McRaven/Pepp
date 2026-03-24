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

  inline bool numeric() const noexcept override { return stream_size() <= 8; }
  inline bool signed_numeric() const noexcept override { return false; }
  inline bool text() const noexcept override { return true; }
  inline bool identifier() const noexcept override { return false; }
  inline bool wide() const noexcept override { return stream_size() > 8; }
  inline bool fixed_size() const noexcept override { return false; }
  inline u64 stream_size() const noexcept override { return _size; }
  inline void set_stream_size(u64 size) noexcept override { _size = size; }
  inline u64 min_size() const noexcept override { return _bytes.size(); }
  void value(bits::span<u8> dest, bits::Order destEndian = bits::hostOrder()) const noexcept override;
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

  inline bool numeric() const noexcept override { return true; }
  inline bool signed_numeric() const noexcept override { return false; }
  inline bool text() const noexcept override { return true; }
  inline bool identifier() const noexcept override { return false; }
  inline bool wide() const noexcept override { return false; }
  inline bool fixed_size() const noexcept override { return true; }
  inline u64 stream_size() const noexcept override { return 1; }
  inline void set_stream_size(u64) noexcept override {}
  inline u64 min_size() const noexcept override { return 1; }

  void value(bits::span<u8> dest, bits::Order destEndian = bits::hostOrder()) const noexcept override;
  std::string string() const override;
  std::string raw_string() const override;

private:
  char _value = {};
};
} // namespace pepp::ast
