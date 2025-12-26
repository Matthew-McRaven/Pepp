/*
 * Copyright (c) 2024-2026 J. Stanley Warford, Matthew McRaven
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
#include "textutils.hpp"

QString removeLeading0(const QString &str) {
  for (int it = 0; it < str.size(); it++) {
    if (str.at(it) != '0') return str.mid(it);
  }
  // Should be unreacheable, but here for safety.
  return str;
}

QStringView rtrimmed(const QString &str) {
  // Perform right-strip of string. `QString::trimmed() const` trims both ends.
  qsizetype lastIndex = str.size() - 1;
  while (QChar(str[lastIndex]).isSpace() && lastIndex > 0) lastIndex--;
  // If line is all spaces, then the string should be empty.
  if (lastIndex == 0) return QStringView();
  // Otherwise, we need to add 1 to last index to convert index (0-based) to size (1-based).
  return QStringView(str).left(lastIndex + 1);
}
