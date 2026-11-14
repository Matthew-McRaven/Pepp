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

#include "core/math/bitmanip/strings.hpp"

size_t bits::bytesToAsciiHex(span<char> out, span<const u8> in, span<const SeparatorRule> separators) {
  static const u8 chars[] = "0123456789ABCDEF";
  size_t outIt = 0;
  for (size_t inIt = 0; inIt < in.size(); inIt++) {
    if (outIt + 2 <= out.size()) {
      out[outIt++] = chars[(in[inIt] >> 4) & 0x0f];
      out[outIt++] = chars[in[inIt] & 0xf];
    } else break;

    if (outIt + 1 > out.size()) break;
    for (auto &rule : separators) {
      if ((inIt + 1) % rule.modulus == 0) {
        out[outIt++] = rule.separator;
        break;
      }
    }
  }
  return outIt;
}

size_t bits::bytesToPrintableAscii(span<char> out, span<const u8> in, span<const SeparatorRule> separators) {
  size_t outIt = 0;
  for (size_t inIt = 0; inIt < in.size(); inIt++) {
    char i = in[inIt];
    if (outIt + 1 <= out.size()) {
      out[outIt++] = std::isprint(i) ? i : '.';
    } else break;

    if (outIt + 1 > out.size()) break;
    for (auto &rule : separators) {
      if ((inIt + 1) % rule.modulus == 0) {
        out[outIt++] = rule.separator;
        break;
      }
    }
  }
  return outIt;
}

u8 hex_to_int(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}
std::optional<std::vector<u8>> bits::asciiHexToByte(span<const char> in) {
  std::vector<u8> ret = {};
  // We typically expect 3 bytes per octet (hex char, hex char, space), so allocate storage for the common case.
  ret.reserve(in.size() / 3 + 2);
  size_t inIt = 0;
  while (inIt < in.size()) {
    // Consume all whitespace between octets
    if (auto c = in[inIt]; c == ' ' || c == '\t' || c == '\n' || c == '\r') inIt++;
    else if (inIt + 1 >= in.size()) return std::nullopt; // Not enough chars to make a byte.
    else {
      // Parse the next two chars into a single byte. Can't use strtol; there may be no gaps between octets.
      auto span = in.subspan(inIt, 2);
      // If either character is not a hex digit, hex_to_int returns -1 (bitpattern all 1s). By sign-extending the
      // intermediate to 16-bit signed, I can examine the sign bit to check if either char was invalid.
      u16 byte = i16(hex_to_int(span[0]) << 4) | i16(hex_to_int(span[1]));
      if (byte & 0x8000) break;
      ret.push_back(byte);
      inIt += 2;
    }
  }
  return ret;
}

std::optional<bits::ParsedInteger> bits::c_integer(std::string_view word) {
  const bool hex = word.starts_with("0x") || word.starts_with("0X");
  const u8 base = hex ? 16 : 10;
  if (const auto value = from_chars_exact<u64>(hex ? word.substr(2) : word, base)) return ParsedInteger{*value, base};
  return std::nullopt;
}

namespace {
char lower(char c) { return static_cast<char>(c | 0x20); }

// LD supports the following integer literal suffixes.
std::optional<u8> suffix_base(char c) {
  switch (lower(c)) {
  case 'h': [[fallthrough]];
  case 'x': return 16;
  case 'b': return 2;
  case 'o': return 8;
  case 'd': return 10;
  default: return std::nullopt;
  }
}

// The following suffixes multiply the result by 2<<result
std::optional<int> multiplier_shift(std::string_view rest) {
  if (rest.empty()) return 0;
  else if (rest.size() != 1) return std::nullopt;
  switch (lower(rest[0])) {
  case 'k': return 10;
  case 'm': return 20;
  default: return std::nullopt;
  }
}

// $1F, 0x1F, 17, 017 (octal), each optionally followed by K or M.
std::optional<bits::ParsedInteger> ld_prefixed(std::string_view word) {
  u8 base = 10;
  if (word.starts_with('$')) base = 16, word.remove_prefix(1);
  else if (word.starts_with("0x") || word.starts_with("0X")) base = 16, word.remove_prefix(2);
  else if (word.size() > 1 && word[0] == '0') base = 8;
  u64 value = 0;
  const char *const end = word.data() + word.size();
  // Parse the integer minus the $/0x prefix, accounting for the multiplie suffix if present
  const auto [last, ec] = std::from_chars(word.data(), end, value, base);
  const auto shift = ec == std::errc{} ? multiplier_shift(std::string_view(last, end)) : std::nullopt;
  if (!shift || value > (~u64{0} >> *shift)) return std::nullopt;
  return bits::ParsedInteger{value << *shift, base};
}

// 1Fh, 1Fx, 101b, 17o, 25d. Suffixes b/o/d need a leading digit, since `bad` is a name.
std::optional<bits::ParsedInteger> ld_suffixed(std::string_view word) {
  const auto base = word.size() < 2 ? std::nullopt : suffix_base(word.back());
  if (!base || (*base != 16 && (word[0] < '0' || word[0] > '9'))) return std::nullopt;
  const auto value = bits::from_chars_exact<u64>(word.substr(0, word.size() - 1), *base);
  if (!value) return std::nullopt;
  return bits::ParsedInteger{*value, *base};
}
} // namespace

std::optional<bits::ParsedInteger> bits::ld_integer(std::string_view word) {
  if (const auto prefixed = ld_prefixed(word)) return prefixed;
  return ld_suffixed(word);
}
