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

#include "bts/bitmanip/copy.hpp"

void bits::memcpy_endian(std::span<uint8_t> dest, Order destOrder, std::span<const uint8_t> src, Order srcOrder) {
  // At most 1 offset will be used at a time, determined by which pointer is
  // longer.
  std::size_t srcOffset = 0, destOffset = 0;
  // If src is big endian, we need any 0's to appear on left.
  // src must be shifted when it is longer than dest for 0's to remain.
  if (src.size() > dest.size() && srcOrder == Order::BigEndian) srcOffset = src.size() - dest.size();
  // Ibid with src and dest reversed.
  else if (dest.size() > src.size() && destOrder == Order::BigEndian) destOffset = dest.size() - src.size();

  // length now points past end of src, must be adjusted
  auto adjustedDest = dest.subspan(destOffset);
  // Src is iterated from start to end, so it must contain no more than
  // adjustedDest elements.
  auto adjustedSrc = src.subspan(srcOffset, std::min(src.size() - srcOffset, dest.size()));

  if (srcOrder == destOrder) std::copy(adjustedSrc.begin(), adjustedSrc.end(), adjustedDest.begin());
  else std::reverse_copy(adjustedSrc.begin(), adjustedSrc.end(), adjustedDest.begin());
}

// TODO: might be able to vectorize this for large lens.
void bits::memcpy_xor(bits::span<uint8_t> dest, bits::span<const uint8_t> src1, bits::span<const uint8_t> src2) {
  auto len = std::min(dest.size_bytes(), std::min(src1.size_bytes(), src2.size_bytes()));
  for (auto it = 0; it < len; it++) dest[it] = src1[it] ^ src2[it];
}
