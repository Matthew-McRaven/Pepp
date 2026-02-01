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

#include <algorithm>
#include <bit>
#include <concepts>
#include <cstring> // for memcpy.
#include <ranges>
#include "../../integers.h"
#include "core/math/bitmanip/order.hpp"
namespace bits {
namespace detail {
// Sample code from: https://en.cppreference.com/w/cpp/numeric/bit_cast
// Needed since CI environment doesn't have bitcast...
template <class To, class From>
std::enable_if_t<sizeof(To) == sizeof(From) && std::is_trivially_copyable_v<From> && std::is_trivially_copyable_v<To>,
                 To>
// constexpr support needs compiler magic
bit_cast(const From &src) noexcept {
  static_assert(std::is_trivially_constructible_v<To>, "This implementation additionally requires "
                                                       "destination type to be trivially constructible");

  To dst;
  std::memcpy(&dst, &src, sizeof(To));
  return dst;
}
} // namespace detail
#if __cpp_lib_bit_cast == 201806L
using ::std::bit_cast;
#else
using detail::bit_cast;
#endif

#ifdef PEPP_HAS_RANGES_REVERSE
// Use (better) range version when possible
template <std::integral T> constexpr T byteswap(T value) noexcept {
  // Sample code from: https://en.cppreference.com/w/cpp/numeric/byteswap
  // Waiting on my compiler to support byteswap...
  static_assert(std::has_unique_object_representations_v<T>, "T may not have padding bits");
  auto value_representation = bit_cast<std::array<std::byte, sizeof(T)>>(value);
  std::ranges::reverse(value_representation);
  return bit_cast<T>(value_representation);
}
#else
#if defined(_MSC_VER)
inline u16 bswap16(u16 value) noexcept { return _byteswap_ushort(value); }
inline u32 bswap32(u32 value) noexcept { return _byteswap_ulong(value); }
inline u64 bswap64(u64 value) noexcept { return _byteswap_uint64(value); }
#else
constexpr u16 bswap16(u16 value) noexcept { return __builtin_bswap16(value); }
constexpr u32 bswap32(u32 value) noexcept { return __builtin_bswap32(value); }
constexpr u64 bswap64(u64 value) noexcept { return __builtin_bswap64(value); }
#endif

// Fallback to compiler intrinsics, then fallback to bitcast+reverse.
template <std::integral T> constexpr T byteswap(T value) noexcept {
  static_assert(std::has_unique_object_representations_v<T>, "T may not have padding bits");
  if constexpr (sizeof(T) == 2) return T(bswap16(value));
  else if constexpr (sizeof(T) == 4) return T(bswap32(value));
  else if constexpr (sizeof(T) == 8) return T(bswap64(value));
  else {
    // Fallback implementation for non-standard sizes.
    auto value_representation = bit_cast<std::array<std::byte, sizeof(T)>>(value);
    std::reverse(value_representation.begin(), value_representation.end());
    return bit_cast<T>(value_representation);
  }
}
#endif
} // namespace bits
