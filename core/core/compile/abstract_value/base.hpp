/*
 * Copyright (c) 2023-2026 J. Stanley Warford, Matthew McRaven
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
#include "core/integers.h"
#include "core/math/bitmanip/order.hpp"
#include "core/math/bitmanip/span.hpp"

namespace pepp::ast {

class BaseValue {
public:
  explicit BaseValue() noexcept = default;
  virtual ~BaseValue() noexcept = default;
  friend void swap(BaseValue &, BaseValue &) noexcept { using std::swap; }
  // Does the argument make sense as a u64?
  virtual bool numeric() const noexcept = 0;
  // If interpreted number, should the value be treated as signed?
  virtual bool signed_numeric() const noexcept = 0;
  // Should the argument be interpreted as ASCII/UTF-8 text?
  virtual bool text() const noexcept = 0;
  // Is the argument an unquoted string that is not interpreted as a symbol?
  virtual bool identifier() const noexcept = 0;
  // Does the argument fit in a u64?
  virtual bool wide() const noexcept = 0;
  // Are all instances of this value type the same size in the bitstream?   // e.g., not the case for strings
  virtual bool fixed_size() const noexcept = 0;
  // Number of bytes to be allocated in the bitstream.
  virtual u64 stream_size() const noexcept = 0;
  // Change the number of bytes allocated in the bitstream, truncating if smaller than min_size
  virtual void set_stream_size(u64 size) noexcept = 0;
  // Minimum number of bytes to represent value. e.g., If storing a u64 with value 128, min_size is 1.
  virtual u64 min_size() const noexcept = 0;
  virtual void value(bits::span<u8> dest, bits::Order destEndian = bits::Order::BigEndian) const noexcept = 0;
  virtual std::string string() const = 0;
  // like string(), except without quotation marks.
  virtual std::string raw_string() const = 0;
  // Helper to extract an truncated integer value without creating the buffer yourself.
  template <std::integral I> I value_as(bits::Order destEndian = bits::hostOrder()) const noexcept {
    I ret = 0;
    value(bits::span<u8>{(u8 *)&ret, sizeof(ret)}, destEndian);
    return ret;
  }
};
} // namespace pepp::ast
