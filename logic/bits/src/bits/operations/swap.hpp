#pragma once
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
// Sample code from: https://en.cppreference.com/w/cpp/numeric/byteswap
// Waiting on my compiler to support byteswap...
template <std::integral T> constexpr T byteswap(T value) noexcept {
  static_assert(std::has_unique_object_representations_v<T>,
                "T may not have padding bits");
#if __cpp_lib_bit_cast == 201806L
  auto value_representation =
      std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
  std::ranges::reverse(value_representation);
  return std::bit_cast<T>(value_representation);
#else
  auto value_representation =
      detail::bit_cast<std::array<std::byte, sizeof(T)>>(value);
  std::ranges::reverse(value_representation);
  return detail::bit_cast<T>(value_representation);
#endif
}
} // namespace bits
