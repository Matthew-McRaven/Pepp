#include "fnv.hpp"

// All overloads use the same implementation while selecting an appropriate seed.
u64 pepp::fnv_1a(bits::span<const char> s, u64 seed) noexcept {
  std::size_t h = seed;
  constexpr auto prime = detail::FNV_prime();

  for (unsigned char c : s) {
    h ^= static_cast<std::size_t>(c);
    h *= prime;
  }
  return h;
}

u64 pepp::fnv_1a(bits::span<const u8> s, u64 seed) noexcept {
  return fnv_1a(bits::span<const char>{reinterpret_cast<const char *>(s.data()), s.size()}, seed);
}

u64 pepp::fnv_1a(std::string_view s, u64 seed) noexcept { return fnv_1a(bits::span<const char>{s}, seed); }

u64 pepp::fnv_1a(bits::span<const char> s) noexcept { return fnv_1a(s, fnv_1a_basis()); }

u64 pepp::fnv_1a(bits::span<const u8> s) noexcept { return fnv_1a(s, fnv_1a_basis()); }

u64 pepp::fnv_1a(std::string_view s) noexcept { return fnv_1a(s, fnv_1a_basis()); }
