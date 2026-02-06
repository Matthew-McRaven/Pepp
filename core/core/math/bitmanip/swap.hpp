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

namespace detail {
// Magic C handed to use by: https://graphics.stanford.edu/~seander/bithacks.html
constexpr inline uint16_t bitreverse_impl(uint8_t b) noexcept {
  b = (b >> 4) | (b << 4);
  b = ((b & 0xCC) >> 2) | ((b & 0x33) << 2);
  b = ((b & 0xAA) >> 1) | ((b & 0x55) << 1);
  return b;
}
constexpr inline uint16_t bitreverse_impl(uint16_t b) noexcept {
  // swap odd and even bits
  b = (uint16_t)(((b >> 1) & 0x5555u) | ((b & 0x5555u) << 1));
  // swap consecutive pairs
  b = (uint16_t)(((b >> 2) & 0x3333u) | ((b & 0x3333u) << 2));
  // swap nibbles
  b = (uint16_t)(((b >> 4) & 0x0F0Fu) | ((b & 0x0F0Fu) << 4));
  // swap bytes
  b = (uint16_t)((b >> 8) | (b << 8));
  return b;
}
constexpr inline uint32_t bitreverse_impl(uint32_t b) noexcept {
  // swap odd and even bits
  b = ((b >> 1) & 0x55555555) | ((b & 0x55555555) << 1);
  // swap consecutive pairs
  b = ((b >> 2) & 0x33333333) | ((b & 0x33333333) << 2);
  // swap nibbles ...
  b = ((b >> 4) & 0x0F0F0F0F) | ((b & 0x0F0F0F0F) << 4);
  // swap bytes
  b = ((b >> 8) & 0x00FF00FF) | ((b & 0x00FF00FF) << 8);
  // swap 2-byte long pairs
  b = (b >> 16) | (b << 16);
  return b;
}
constexpr inline uint64_t bitreverse_impl(uint64_t b) noexcept {
  // swap odd and even bits
  b = ((b >> 1) & 0x5555555555555555ULL) | ((b & 0x5555555555555555ULL) << 1);
  // swap consecutive pairs
  b = ((b >> 2) & 0x3333333333333333ULL) | ((b & 0x3333333333333333ULL) << 2);
  // swap nibbles
  b = ((b >> 4) & 0x0F0F0F0F0F0F0F0FULL) | ((b & 0x0F0F0F0F0F0F0F0FULL) << 4);
  // swap bytes
  b = ((b >> 8) & 0x00FF00FF00FF00FFULL) | ((b & 0x00FF00FF00FF00FFULL) << 8);
  // swap 2-byte pairs
  b = ((b >> 16) & 0x0000FFFF0000FFFFULL) | ((b & 0x0000FFFF0000FFFFULL) << 16);
  // swap 4-byte pairs
  b = (b >> 32) | (b << 32);
  return b;
}
} // namespace detail

#if defined(__has_builtin)
#if __has_builtin(__builtin_bitreverse8)
#define HAVE_BUILTIN_BITREVERSE8 1
#endif
#if __has_builtin(__builtin_bitreverse16)
#define HAVE_BUILTIN_BITREVERSE16 1
#endif
#if __has_builtin(__builtin_bitreverse32)
#define HAVE_BUILTIN_BITREVERSE32 1
#endif
#if __has_builtin(__builtin_bitreverse64)
#define HAVE_BUILTIN_BITREVERSE64 1
#endif
#endif

// Should have an intrinsic on most compilers, but provide a fallback just in case.
// Provided as a bunch of 1-off overloads rather than a single template (like byteswap), since
// we need to check for the existence of each intrinsic separately.
inline uint8_t bitreverse(uint8_t x) noexcept {
#if defined(HAVE_BUILTIN_BITREVERSE8)
  return __builtin_bitreverse8(x);
#else
  return detail::bitreverse_impl(x);
#endif
}

inline uint16_t bitreverse(uint16_t x) noexcept {
#if defined(HAVE_BUILTIN_BITREVERSE16)
  return __builtin_bitreverse16(x);
#else
  return detail::bitreverse_impl(x);
#endif
}

inline uint32_t bitreverse(uint32_t x) noexcept {
#if defined(HAVE_BUILTIN_BITREVERSE32)
  return __builtin_bitreverse32(x);
#else
  return detail::bitreverse_impl(x);
#endif
}

inline uint64_t bitreverse(uint64_t b) noexcept {
#if defined(HAVE_BUILTIN_BITREVERSE64)
  return __builtin_bitreverse64(b);
#else
  return detail::bitreverse_impl(b);
#endif
}

} // namespace bits
