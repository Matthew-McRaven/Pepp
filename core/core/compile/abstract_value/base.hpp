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

  // Minimum number of bytes to encode the value. E.g., a (u64) 128 has a nominal size of 8 but a min size of 1.
  virtual u64 serialized_size() const noexcept = 0;
  [[nodiscard]] virtual u32 serialize(bits::span<u8> dest, bits::Order destEndian = bits::Order::BigEndian,
                                      u32 max_size = (u32)-1) const noexcept = 0;

  virtual std::string string() const = 0;
  // like string(), except without quotation marks.
  virtual std::string raw_string() const = 0;

  // Helper to extract an truncated integer value without creating the buffer yourself.
  template <std::integral I> I value_as(bits::Order destEndian = bits::hostOrder()) const noexcept {
    I ret = 0;
    (void)serialize(bits::span<u8>{(u8 *)&ret, sizeof(ret)}, destEndian, sizeof(I));
    return ret;
  }
};
} // namespace pepp::ast
