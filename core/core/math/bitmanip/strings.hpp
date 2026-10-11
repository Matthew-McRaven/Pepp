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
#include <algorithm>
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>
#include "../../integers.h"
#include "core/math/bitmanip/span.hpp"
#include "fmt/format.h"

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
      value = '\\';
    } else if (head == '\'') { // Escaped single quote
      value = '\'';
    } else if (head == '"') { // Escaped double quote
      value = '"';
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
      // Exactly two hex digits
      const auto hex = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        else if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        else return -1;
      };
      if (end - start < 2) return false;
      const int hi = hex(*start++), lo = hex(*start++);
      if (hi < 0 || lo < 0) return false;
      value = static_cast<u8>(hi * 16 + lo);
    } else return false; // Unknown escape sequence.
  } else {
    value = head;
  }
  return true;
}

// The byte denoted by text, which must be exactly one character or escape sequence, e.g. the text between the quotes of
// a character constant.
inline std::optional<u8> escapedToByte(std::string_view text) {
  auto start = text.begin(), end = text.end();
  u8 value = 0;
  if (!charactersToByte(start, end, value) || start != end) return std::nullopt;
  return value;
}

template <typename OutputIt> OutputIt byteToEscaped(u8 value, OutputIt out) {
  switch (value) {
  case '\\': return *out++ = '\\', *out++ = '\\', out;
  case '\b': return *out++ = '\\', *out++ = 'b', out;
  case '\f': return *out++ = '\\', *out++ = 'f', out;
  case '\n': return *out++ = '\\', *out++ = 'n', out;
  case '\r': return *out++ = '\\', *out++ = 'r', out;
  case '\t': return *out++ = '\\', *out++ = 't', out;
  case '\v': return *out++ = '\\', *out++ = 'v', out;
  case '\0': return *out++ = '\\', *out++ = '0', out;
  }
  if (value >= 0x20 && value < 0x7F) return *out++ = static_cast<char>(value), out;
  else return fmt::format_to(out, "\\x{:02X}", value);
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

inline std::string to_upper(std::string_view s) {
  std::string s_copy{s};
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
inline std::string to_lower(std::string_view s) {
  std::string s_copy{s};
  to_lower_inplace(s_copy);
  return s_copy;
}

inline std::string_view chopped(std::string_view sv, std::size_t n) {
  if (n >= sv.size()) return {};
  return sv.substr(0, sv.size() - n);
}

inline bool contains(std::string_view haystack, std::string_view needle) {
  return haystack.find(needle) != std::string_view::npos;
}

// Right-strip whitespace
inline std::string_view rtrimmed_view(const std::string &str) {
  std::size_t size = str.size();
  while (size > 0 && std::isspace((u8)str[size - 1])) size--;
  return std::string_view(str).substr(0, size);
}

inline std::string rtrimmed(const std::string &str) { return std::string{rtrimmed_view(str)}; }

} // namespace bits
