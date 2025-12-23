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
#include "./read.hpp"

std::optional<QString> about::detail::readFile(QString fname) {
  QFile file(fname);
  QString fileText;
  if (file.open(QFile::ReadOnly)) fileText = file.readAll();
  else {
    qWarning() << "Failed to open: " << fname << ".\n";
    return std::nullopt;
  }
  return fileText;
}

about::detail::ReadHelper::ReadHelper(QObject *parent) : QObject(parent) {}

QString about::detail::ReadHelper::readFile(QString fname) {
  auto ret = about::detail::readFile(fname);
  return ret.value_or("");
}
