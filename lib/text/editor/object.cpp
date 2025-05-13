/*
 * Copyright (c) 2024 J. Stanley Warford, Matthew McRaven
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

#include "./object.hpp"
#include <QRegularExpression>

ObjectUtilities::ObjectUtilities(QObject *parent) : QObject(parent) {}

bool ObjectUtilities::valid(int key) {
  static const QSet<int> valids = {
      Qt::Key_0,    Qt::Key_1, Qt::Key_2,         Qt::Key_3,      Qt::Key_4,    Qt::Key_5,     Qt::Key_6,
      Qt::Key_7,    Qt::Key_8, Qt::Key_9,         Qt::Key_A,      Qt::Key_B,    Qt::Key_C,     Qt::Key_D,
      Qt::Key_E,    Qt::Key_F, Qt::Key_Backspace, Qt::Key_Delete, Qt::Key_Left, Qt::Key_Right, Qt::Key_Up,
      Qt::Key_Down, Qt::Key_Z, Qt::Key_Space,     Qt::Key_Return, Qt::Key_Enter};
  return valids.contains(key);
}

QString ObjectUtilities::format(QString input, bool includeZZ) const {
  // Remove non-hex characters, the zz sentinel, and any spaces.
  static const auto re = QRegularExpression(R"(([^0-9a-fA-F]|[zZ]|\s)+)");
  static const auto space = [](int offset, int bytes) { return (offset % bytes == 0) ? '\n' : ' '; };
  // Avoid double-replace by merging original zz|\s regex with the one from normalize.
  // Failure to remove non-hex characters results in really weird object code,
  input = input.replace(re, "");
  // If there is an incomplete octet, add space to 0-pad it.
  bool trailingNibble = input.size() % 2 == 1;
  int paddedSize = input.size() + trailingNibble;
  // 1 for space before ZZ if not empty, 2 for "ZZ",
  // plus the length of the string, plus spacing after each pair.
  int bytesForEnd = includeZZ ? (paddedSize > 0 ? 1 : 0) + 2 : 0;
  QString result(bytesForEnd + paddedSize + (paddedSize - 1) / 2, 'X');

  int offset = 0;
  // Don't copy the trailing nibble if present, since we need to pad it to an otcet.
  for (int it = 0; it < input.size() - trailingNibble; it++) {
    result[it + offset] = input[it].toUpper();
    if (it % 2 == 1) {
      // Newline follows last byte in row, not leading the first byte.
      // Pre-increment achieves this effect with less math in spaces(...).
      offset++;
      // If inserting a space would overflow the result, we are are at the end of the string; stop.
      if (it + offset >= result.size()) {
        // Neither shouuld be true, because it would imply we still need to append more data.
        Q_ASSERT(!(includeZZ | trailingNibble));
        break;
      }
      result[it + offset] = space(offset, _bytesPerRow);
    }
  }
  // Result will always be 5 characters long if the input is non-empty.
  // Trailing nibble is impossible if len(input)==0; access will not be OOB.
  if (trailingNibble) {
    result[result.size() - bytesForEnd - 2] = '0';
    result[result.size() - bytesForEnd - 1] = input[input.size() - 1].toUpper();
  }
  // Optionally skip " ZZ", adding ability for text=>byte preprocessing.
  if (includeZZ) {
    // Leading space on ZZ with an empty input looks bad, so supress it.
    if (input.size() != 0)
      // Pre-increment offset to match above.
      result[result.size() - 3] = space(++offset, _bytesPerRow);
    result[result.size() - 2] = 'Z';
    result[result.size() - 1] = 'Z';
  }
  return result;
}

QString ObjectUtilities::normalize(QString input) const {
  static const auto re = QRegularExpression(R"([^0-9a-fA-FzZ]|\s)");
  return input.replace(re, "");
}

void ObjectUtilities::setBytesPerRow(int bytes) {
  if (bytes == _bytesPerRow) return;
  _bytesPerRow = bytes;
  emit bytesPerRowChanged();
}

int ObjectUtilities::bytesPerRow() const { return _bytesPerRow; }
