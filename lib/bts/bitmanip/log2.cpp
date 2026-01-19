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

#include "bts/bitmanip/log2.hpp"
#include <bit>
#include <iostream>
u8 bits::ceil_log2(u64 value) {
  if (value == 0) {
    static const char *const e = "Must be non-0";
    std::cerr << e;
    throw std::logic_error(e);
  }
  u64 ceil = std::bit_ceil(value);
  return sizeof(value) * 8 - std::countl_zero(ceil) - 1;
}

u64 bits::nearest_power_of_two(u64 value) {
  if (value == 0) return 1;
  return std::bit_ceil(value);
}
