#include "fnv.hpp"

u64 pepp::fnv_1a(bits::span<const char> s) noexcept {
  std::size_t h = detail::FNV_offset_basis();
  constexpr auto prime = detail::FNV_prime();

  for (unsigned char c : s) {
    h ^= static_cast<std::size_t>(c);
    h *= prime;
  }
  return h;
}

u64 pepp::fnv_1a(bits::span<const u8> s) noexcept {
  return fnv_1a(bits::span<const char>{reinterpret_cast<const char *>(s.data()), s.size()});
}

u64 pepp::fnv_1a(std::string_view s) noexcept { return fnv_1a(bits::span<const char>{s}); }
