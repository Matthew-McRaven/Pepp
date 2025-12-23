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
#include <QtCore>
#include "utils/bits/span.hpp"
namespace bits {
template <typename Iterator> bool charactersToByte(Iterator &start, Iterator end, uint8_t &value) {
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
      qCritical(e);
      throw std::logic_error(e);
      value = static_cast<uint8_t>('\\');
    }
  } else {
    value = head;
  }
  return true;
}

bool startsWithHexPrefix(const QString &string);
qsizetype escapedStringLength(const QString string);
bool escapedStringToBytes(const QString &string, QByteArray &output);
struct SeparatorRule {
  bool skipFirst;
  char separator;
  quint16 modulus;
};

// Separates every byte with a space.
qsizetype bytesToAsciiHex(span<char> out, span<const quint8> in, QVector<SeparatorRule> separator);
// Copy printable ASCII characters from in to out, inserting separators at the appropriate times.
// Non-printable characters are replaced by a "." / full-stop.
qsizetype bytesToPrintableAscii(span<char> out, span<const quint8> in, QVector<SeparatorRule> separator);
std::optional<QList<quint8>> asciiHexToByte(span<const char> in);
} // namespace bits
