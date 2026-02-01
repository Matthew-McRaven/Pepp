/*
 * /Copyright (c) 2026. Stanley Warford, Matthew McRaven
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "case_insensitive.hpp"
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
