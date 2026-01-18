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
#include <iostream>
#include "bts/bitmanip/integers.h"
#include "bts/bitmanip/span.hpp"

namespace bits {
template <typename Iterator> bool charactersToByte(Iterator &start, Iterator end, u8 &value) {
  // If start == end, then there are no characters to parse!
  if (start == end) {
    return false;
  }
  char head = *start++;
  if (head == '\\') {
    if (start == end) return false;
    head = *start++;
    if (head == '\\') { // Escaped backslash
      head = '\\';
    } else if (head == 'b') { // backspace
      value = 8;
    } else if (head == 'f') { // form feed
      value = 12;
    } else if (head == 'n') { // line feed (new line)
      value = 10;
    } else if (head == 'r') { // carriage return
      value = 13;
    } else if (head == 't') { // horizontal tab
      value = 9;
    } else if (head == 'v') { // vertical tab
      value = 11;
    } else if (head == '0') { // null terminator
      value = 0;
    } else if (head == 'x' || head == 'X') { // hex strings!
      // Need at least two more characters to consume.
      if (end - start < 2) return false;
      else {
        char *end;
        char copied[] = {*(start++), *(start++), '\0'};
        value = strtol(copied, &end, 16);
        if (*end != '\0') return false;
      }
    } else {
      static const char *const e = "Unreachable";
      std::cerr << e;
      throw std::logic_error(e);
      value = static_cast<uint8_t>('\\');
    }
  } else {
    value = head;
  }
  return true;
}

struct SeparatorRule {
  bool skipFirst;
  char separator;
  u16 modulus;
};

// Separates every byte with a space.
size_t bytesToAsciiHex(span<char> out, span<const u8> in, bits::span<const SeparatorRule> separator);
// Copy printable ASCII characters from in to out, inserting separators at the appropriate times.
// Non-printable characters are replaced by a "." / full-stop.
size_t bytesToPrintableAscii(span<char> out, span<const u8> in, bits::span<const SeparatorRule> separator);
std::optional<std::vector<u8>> asciiHexToByte(span<const char> in);

inline void to_upper_inplace(std::string &s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
}

inline std::string to_upper(const std::string &s) {
  std::string s_copy = s;
  to_upper_inplace(s_copy);
  return s_copy;
}

inline void to_lower_inplace(std::string &s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
}

inline std::string to_lower(const std::string &s) {
  std::string s_copy = s;
  to_lower_inplace(s_copy);
  return s_copy;
}
} // namespace bits
