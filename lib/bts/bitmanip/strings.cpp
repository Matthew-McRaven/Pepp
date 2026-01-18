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

#include "strings.hpp"

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
      // If either character is not a hex digit, hex_to_int returns -1 (bitpattern all 1s). By expading the intermediate
      // to 16-bits, I can examine the sign bit to check if either char was invalid.
      u16 byte = hex_to_int(span[0]) << 4 | hex_to_int(span[1]);
      if (byte < 0) break;
      ret.push_back(byte);
      inIt += 2;
    }
  }
  return ret;
}
