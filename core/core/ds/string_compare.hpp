#pragma once

#include <string>
#include <string_view>
#include "core/ds/hash/fnv.hpp"

namespace pepp::bts {

struct ci_char_traits : public std::char_traits<char> {
  static char to_upper(char ch);
  static char to_lower(char ch);
  static bool eq(char c1, char c2);
  static bool lt(char c1, char c2);
  static int compare(const char *s1, const char *s2, std::size_t n);
  static const char *find(const char *s, std::size_t n, char a);
};

using ci_stringview = std::basic_string_view<char, bts::ci_char_traits>;
ci_stringview to_ci_stringview(std::string_view sv) noexcept;

namespace detail {} // namespace detail
// Transparent, case-sensitive hash for std::string / std::string_view.
// Transparent, case-insensitive hash for std::string / std::string_view.
struct cs_hash {
  using is_transparent = void;

  std::size_t operator()(std::string_view s) const noexcept { return std::hash<std::string_view>{}(s); }
  std::size_t operator()(std::string const &s) const noexcept { return (*this)(std::string_view{s}); }
};

struct ci_hash {
  using is_transparent = void;

  std::size_t operator()(std::string_view s) const noexcept { return pepp::case_insensitive_fnv_1a<ci_char_traits>(s); }

  std::size_t operator()(std::string const &s) const noexcept { return (*this)(std::string_view{s}); }
};

// Transparent, case-sensitive equality.
struct cs_eq {
  using is_transparent = void;

  bool operator()(std::string_view a, std::string_view b) const noexcept { return a == b; }
  bool operator()(std::string const &a, std::string const &b) const noexcept {
    return (*this)(std::string_view{a}, std::string_view{b});
  }
  bool operator()(std::string const &a, std::string_view b) const noexcept { return (*this)(std::string_view{a}, b); }
  bool operator()(std::string_view a, std::string const &b) const noexcept { return (*this)(a, std::string_view{b}); }
};

// Transparent, case-insensitive equality.
struct ci_eq {
  using is_transparent = void;

  bool operator()(std::string_view a, std::string_view b) const noexcept {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
      const auto ca = ci_char_traits::to_lower(static_cast<unsigned char>(a[i]));
      const auto cb = ci_char_traits::to_lower(static_cast<unsigned char>(b[i]));
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

} // namespace pepp::core
