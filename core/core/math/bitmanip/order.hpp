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
#include <bit>
#pragma once
namespace bits {
// Describe the endianness of either a host or quest machine.
enum class Order { BigEndian, LittleEndian, NotApplicable };
// Utilities to detect and report the host machine's byte order.
constexpr Order hostOrder() {
#if defined(__cpp_lib_endian) && __cpp_lib_endian >= 201907L
  if constexpr (std::endian::native == std::endian::little) return Order::LittleEndian;
  if constexpr (std::endian::native == std::endian::big) return Order::BigEndian;
  return Order::NotApplicable; // mixed/unknown
// Fallback for older standard libraries: rely on common predefined macros.
#elif defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  return Order::LittleEndian;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  return Order::BigEndian;
#else
  return Order::NotApplicable;
#endif
#else
  // This should never be the case. However, if we can't determine the endianness, the simulator can never work.
  // Fail so that we can determine a better mechanism / workaround for the offending target.
  static_assert(false, "Cannot determine host endianness.");
#endif
}

} // namespace bits
