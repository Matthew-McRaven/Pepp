#include "core/libs/types/case_insensitive.hpp"
pepp::bts::ci_stringview pepp::bts::to_ci_stringview(std::string_view sv) noexcept {
  return ci_stringview{sv.data(), sv.size()};
}

/*
 * Implementation from: https://en.cppreference.com/w/cpp/string/char_traits.html
 */
char pepp::bts::ci_char_traits::to_upper(char ch) { return std::toupper((unsigned char)ch); }

char pepp::bts::ci_char_traits::to_lower(char ch) { return std::tolower((unsigned char)ch); }

bool pepp::bts::ci_char_traits::eq(char c1, char c2) { return to_upper(c1) == to_upper(c2); }

bool pepp::bts::ci_char_traits::lt(char c1, char c2) { return to_upper(c1) < to_upper(c2); }

int pepp::bts::ci_char_traits::compare(const char *s1, const char *s2, std::size_t n) {
  while (n-- != 0) {
    if (to_upper(*s1) < to_upper(*s2)) return -1;
    if (to_upper(*s1) > to_upper(*s2)) return 1;
    ++s1;
    ++s2;
  }
  return 0;
}

const char *pepp::bts::ci_char_traits::find(const char *s, std::size_t n, char a) {
  const auto ua{to_upper(a)};
  while (n-- != 0) {
    if (to_upper(*s) == ua) return s;
    s++;
  }
  return nullptr;
}
