#pragma once

#include <string>
#include <string_view>

namespace pepp::bts {
namespace detail {
static constexpr unsigned char ascii_tolower(unsigned char c) noexcept {
  return (c >= 'A' && c <= 'Z') ? static_cast<unsigned char>(c + ('a' - 'A')) : c;
}
} // namespace detail
// Transparent, case-insensitive hash for std::string / std::string_view.
struct ci_hash {
  using is_transparent = void;

  std::size_t operator()(std::string_view s) const noexcept {
    // FNV-1a (64-bit on 64-bit platforms); any stable hash is fine.
    std::size_t h = sizeof(std::size_t) == 8 ? static_cast<std::size_t>(1469598103934665603ull)
                                             : static_cast<std::size_t>(2166136261u);

    constexpr std::size_t prime =
        sizeof(std::size_t) == 8 ? static_cast<std::size_t>(1099511628211ull) : static_cast<std::size_t>(16777619u);

    for (unsigned char c : s) {
      h ^= static_cast<std::size_t>(detail::ascii_tolower(c));
      h *= prime;
    }
    return h;
  }

  std::size_t operator()(std::string const &s) const noexcept { return (*this)(std::string_view{s}); }
};

// Transparent, case-insensitive equality.
struct ci_eq {
  using is_transparent = void;

  bool operator()(std::string_view a, std::string_view b) const noexcept {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
      const auto ca = detail::ascii_tolower(static_cast<unsigned char>(a[i]));
      const auto cb = detail::ascii_tolower(static_cast<unsigned char>(b[i]));
      if (ca != cb) return false;
    }
    return true;
  }

  bool operator()(std::string const &a, std::string const &b) const noexcept {
    return (*this)(std::string_view{a}, std::string_view{b});
  }
  bool operator()(std::string const &a, std::string_view b) const noexcept { return (*this)(std::string_view{a}, b); }
  bool operator()(std::string_view a, std::string const &b) const noexcept { return (*this)(a, std::string_view{b}); }
};
} // namespace pepp::bts
