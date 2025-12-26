/*
 * Copyright (c) 2023-2026 J. Stanley Warford, Matthew McRaven
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

#include "ls-imgfmt.hpp"
#include <QByteArrayList>
#include <QImageReader>
#include <iostream>

ListImageFormatsTask::ListImageFormatsTask(QObject *parent) : Task(parent) {}

void ListImageFormatsTask::run() {
  const QByteArrayList fmts = QImageReader::supportedImageFormats();
  for (const auto &ba : fmts) std::cout << " " << ba.constData();
  std::cout << "\n";
  std::cout.flush();
  return emit finished(0);
}
