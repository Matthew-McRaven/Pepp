/*
 * Copyright (c) 2023-2026 J. Stanley Warford, Matthew McRaven
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

namespace bits {
// Convert a byte count to a mask.
// e.g., 1=>0xFF
// 2=> 0xFFFF
constexpr uint64_t mask(uint8_t byteCount) { return byteCount >= 8 ? ~0ull : (1ull << (byteCount * 8)) - 1; }
// Sign-extend the low byteCount bytes of value to 64 bits.
constexpr uint64_t sign_extend(uint64_t value, uint8_t byteCount) {
  if (byteCount >= 8) return value;
  const uint64_t sign = 1ull << (byteCount * 8 - 1);
  return ((value & mask(byteCount)) ^ sign) - sign;
}
struct MaskedBits {
  u8 byteCount = 0;
  u64 bitPattern = 0, mask = 0;
  u64 operator()();
  bool operator==(const MaskedBits &other) const;
};
} // namespace bits
