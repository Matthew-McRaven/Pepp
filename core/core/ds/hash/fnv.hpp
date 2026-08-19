#pragma once
#include <string_view>
#include "core/integers.h"
#include "core/math/bitmanip/span.hpp"

namespace pepp {

namespace detail {
// Constants extracted from public domain codeL https://github.com/lcn2/fnv
constexpr std::size_t FNV_offset_basis() noexcept {
  if constexpr (sizeof(std::size_t) == 8) return 0xcbf29ce484222325;
  else return 0x811c9dc5;
}
constexpr std::size_t FNV_prime() noexcept {
  if constexpr (sizeof(std::size_t) == 8) return 0x100000001b3;
  else return 0x01000193;
}
} // namespace detail
// Exposed initial state so that users can "stream" a hash.
constexpr u64 fnv_1a_basis() noexcept { return detail::FNV_offset_basis(); }

u64 fnv_1a(bits::span<const char>) noexcept;
u64 fnv_1a(bits::span<const u8>) noexcept;
u64 fnv_1a(std::string_view) noexcept;
template <std::integral I> u64 fnv_1a(I value) noexcept {
  return fnv_1a(bits::span<const char>{reinterpret_cast<const char *>(&value), sizeof(I)});
}
// Streaming form: continue from `seed` rather than starting over, so folding a sequence in one piece at a time gives
// the same result as hashing it in a single call. fnv_1a(x) is fnv_1a(x, fnv_1a_basis()).
u64 fnv_1a(bits::span<const char>, u64 seed) noexcept;
u64 fnv_1a(bits::span<const u8>, u64 seed) noexcept;
u64 fnv_1a(std::string_view, u64 seed) noexcept;
template <std::integral I> u64 fnv_1a(I value, u64 seed) noexcept {
  return fnv_1a(bits::span<const char>{reinterpret_cast<const char *>(&value), sizeof(I)}, seed);
}

template <typename char_traits> u64 case_insensitive_fnv_1a(bits::span<const char> s) noexcept {
  std::size_t h = detail::FNV_offset_basis();
  constexpr auto prime = detail::FNV_prime();

  for (unsigned char c : s) {
    h ^= static_cast<std::size_t>(char_traits::to_lower(c));
    h *= prime;
  }
  return h;
}
template <typename char_traits> u64 case_insensitive_fnv_1a(std::string_view s) noexcept {
  return case_insensitive_fnv_1a<char_traits>(bits::span<const char>{s});
}

} // namespace pepp
