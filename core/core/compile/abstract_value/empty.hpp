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
#include "core/compile/abstract_value/base.hpp"

namespace pepp::ast {

struct Empty : public BaseValue {
public:
  explicit Empty();
  explicit Empty(u8 size);
  Empty(const Empty &other);
  Empty(Empty &&other) noexcept;
  Empty &operator=(Empty other);
  friend void swap(Empty &first, Empty &second) {
    using std::swap;
    swap(first._size, second._size);
  }
  inline bool numeric() const noexcept override { return true; }
  inline bool signed_numeric() const noexcept override { return false; }
  inline bool text() const noexcept override { return false; }
  inline bool identifier() const noexcept override { return false; }
  inline bool wide() const noexcept override { return _size < 8; }
  inline bool fixed_size() const noexcept override { return false; }
  inline u64 stream_size() const noexcept override { return _size; }
  inline void set_stream_size(u64 size) noexcept override { _size = size; }
  inline u64 min_size() const noexcept override { return _size; };
  void value(bits::span<u8> dest, bits::Order targetEndian = bits::hostOrder()) const noexcept override;

  std::string string() const override { return ""; }
  std::string raw_string() const override { return ""; }

private:
  u8 _size = 0;
};

} // namespace pepp::ast
