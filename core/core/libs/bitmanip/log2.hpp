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
#include "core/libs/bitmanip/integers.h"

namespace bits {
/*
 * 01 => 0
 * 02 => 1
 * 04 => 2
 * 08 => 3
 * 16 => 4
 * 32 => 5
 * etc
 *
 *Will throw if value == 0.
 */
u8 ceil_log2(u64 value);
/*
 * 01 => 0
 * 02 => 1
 * 03 => 2
 * 04 => 2
 * 08 => 3
 * 16 => 4
 * 31 => 5
 * 32 => 5
 * etc
 */
u64 nearest_power_of_two(u64 value);

static constexpr u64 ceil_div(u64 a, size_t b) noexcept { return (a + b - 1) / b; }
// Both assum power-of-two aligns
static constexpr std::uintptr_t align_down(std::uintptr_t x, std::size_t a) { return x & ~(std::uintptr_t(a) - 1); }
static constexpr std::uintptr_t align_up(std::uintptr_t x, std::size_t a) {
  if (a <= 1) return x;
  else return (x + (a - 1)) & ~(std::uintptr_t(a) - 1);
}
} // namespace bits
