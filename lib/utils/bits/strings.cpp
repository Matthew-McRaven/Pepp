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

bool bits::startsWithHexPrefix(const QString &string) { return string.startsWith("0x") || string.startsWith("0X"); }

qsizetype bits::escapedStringLength(const QString string) {
  auto asUTF = string.toUtf8();
  auto start = asUTF.begin();
  bool okay = true;
  size_t accumulated_size = 0;
  uint8_t _;
  while (start != asUTF.end()) {
    okay &= bits::charactersToByte(start, asUTF.end(), _);
    accumulated_size++;
  }
  if (!okay) throw std::logic_error("Not a valid string!");
  return accumulated_size;
}

bool bits::escapedStringToBytes(const QString &string, QByteArray &output) {
  auto asUTF = string.toUtf8();
  auto start = asUTF.begin();
  bool okay = true;
  uint8_t temp = 0;
  while (start != asUTF.end()) {
    okay &= bits::charactersToByte(start, asUTF.end(), temp);
    output.push_back(temp);
  }
  return okay;
}

qsizetype bits::bytesToAsciiHex(span<char> out, span<const quint8> in, QVector<SeparatorRule> separators) {
  static const quint8 chars[] = "0123456789ABCDEF";
  qsizetype outIt = 0;
  for (int inIt = 0; inIt < in.size(); inIt++) {
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

qsizetype bits::bytesToPrintableAscii(span<char> out, span<const quint8> in, QVector<SeparatorRule> separators) {
  qsizetype outIt = 0;
  for (int inIt = 0; inIt < in.size(); inIt++) {
    char i = in[inIt];
    if (outIt + 1 <= out.size()) {
      out[outIt++] = QChar::isPrint(i) ? i : '.';
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

qint8 hex_to_int(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}
std::optional<QList<quint8>> bits::asciiHexToByte(span<const char> in) {
  QList<quint8> ret = {};
  // We typically expect 3 bytes per octet (hex char, hex char, space), so allocate storage for the common case.
  ret.reserve(in.size() / 3 + 2);
  qsizetype inIt = 0;
  while (inIt < in.size()) {
    // Consume all whitespace between octets
    if (auto c = in[inIt]; c == ' ' || c == '\t' || c == '\n' || c == '\r') inIt++;
    else if (inIt + 1 >= in.size()) return std::nullopt; // Not enough chars to make a byte.
    else {
      // Parse the next two chars into a single byte. Can't use strtol; there may be no gaps between octets.
      auto span = in.subspan(inIt, 2);
      // If either character is not a hex digit, hex_to_int returns -1 (bitpattern all 1s). By expading the intermediate
      // to 16-bits, I can examine the sign bit to check if either char was invalid.
      qint16 byte = hex_to_int(span[0]) << 4 | hex_to_int(span[1]);
      if (byte < 0) break;
      ret.push_back(byte);
      inIt += 2;
    }
  }
  return ret;
}
