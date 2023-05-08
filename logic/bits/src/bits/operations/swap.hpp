#pragma once
#include "bits/order.hpp"
#include <QtEndian>
#include <algorithm>
#include <bit>
#include <concepts>
#include <cstring> // for memcpy.
#include <ranges>
namespace bits {
namespace detail {
// Sample code from: https://en.cppreference.com/w/cpp/numeric/bit_cast
// Needed since CI environment doesn't have bitcast...
template <class To, class From>
std::enable_if_t<sizeof(To) == sizeof(From) &&
                     std::is_trivially_copyable_v<From> &&
                     std::is_trivially_copyable_v<To>,
                 To>
// constexpr support needs compiler magic
bit_cast(const From &src) noexcept {
  static_assert(std::is_trivially_constructible_v<To>,
                "This implementation additionally requires "
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
#if __cpp_lib_byteswap == 202110L
using ::std::byteswap;
#else
namespace detail {
template <typename T, typename = T> struct HasRangesReverse : std::false_type {
  using value = T;
};

template <typename T>
struct HasRangesReverse<T, decltype(&std::ranges::reverse, T())>
    : std::true_type {};
} // namespace detail

// Use (better) range version when possible
template <std::integral T>
constexpr typename std::enable_if<detail::HasRangesReverse<T>::value, T>::type
byteswap(T value) noexcept {
  // Sample code from: https://en.cppreference.com/w/cpp/numeric/byteswap
  // Waiting on my compiler to support byteswap...
  static_assert(std::has_unique_object_representations_v<T>,
                "T may not have padding bits");
  auto value_representation = bit_cast<std::array<std::byte, sizeof(T)>>(value);
  std::ranges::reverse(value_representation);
  return bit_cast<T>(value_representation);
}

// Otherwise use fallback Qt implementation.
template <std::integral T>
constexpr typename std::enable_if<!detail::HasRangesReverse<T>::value, T>::type
byteswap(T value) noexcept {
  static_assert(std::has_unique_object_representations_v<T>,
                "T may not have padding bits");
  if (bits::hostOrder() == bits::Order::LittleEndian)
    return qToBigEndian(value);
  else
    return qToLittleEndian(value);
}
#endif
} // namespace bits
