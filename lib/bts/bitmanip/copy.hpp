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
#include <cstring>
#include "bts/bitmanip/order.hpp"
#include "bts/bitmanip/span.hpp"

namespace bits {
template <typename T, typename U, std::size_t destSize = std::dynamic_extent, std::size_t srcSize = std::dynamic_extent>
void memcpy(bits::span<T, destSize> dest, bits::span<const U, srcSize> src) {
  auto len = std::min(dest.size_bytes(), src.size_bytes());
  ::memcpy(dest.data(), src.data(), len);
}
template <typename T, std::size_t size = std::dynamic_extent> void memclr(bits::span<T, size> dest) {
  memset(dest.data(), 0, dest.size_bytes());
}

template <std::integral T, std::integral U> T memcpy_endian(U src) {
  return memcpy_endian<T>({&src, sizeof(U)}, bits::hostOrder());
}

// When src is longer than dest, truncates high-order bytes (like casting
// u16->u8). When dest is longer than src, dest is 0-padded.
void memcpy_endian(span<uint8_t> dest, Order destOrder, span<const uint8_t> src, Order srcOrder);

template <std::integral T> T memcpy_endian(span<const uint8_t> src, Order srcOrder) {
  T dest = 0;
  memcpy_endian(span<uint8_t>{reinterpret_cast<uint8_t *>(&dest), sizeof(T)}, bits::hostOrder(), src, srcOrder);
  return dest;
}

template <std::integral T> void memcpy_endian(span<uint8_t> dest, Order destOrder, T src) {
  memcpy_endian(dest, destOrder, span{reinterpret_cast<const uint8_t *>(&src), sizeof(T)}, bits::hostOrder());
}

void memcpy_xor(bits::span<uint8_t> dest, bits::span<const uint8_t> src1, bits::span<const uint8_t> src2);
} // namespace bits
