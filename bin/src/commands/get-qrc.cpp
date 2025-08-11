/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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

#include "get-qrc.hpp"
#include <iostream>

GetQRCTask::GetQRCTask(QString source, QString dest, QObject *parent) : Task(parent), _src(source), _dst(dest) {}

void GetQRCTask::run() {
  QString adjusted = _src;
  if (!_src.startsWith(":/")) adjusted = ":/" + _src;
  QFileInfo src_info(adjusted);
  QFileInfo dst_info(_dst);
  // Make relative paths relative to $PWD.
  if (!dst_info.isAbsolute()) dst_info = QFileInfo(QDir::current(), _dst);

  if (!QFile::copy(src_info.absoluteFilePath(), dst_info.absoluteFilePath())) {
    std::cerr << "Error copying file: " << src_info.absoluteFilePath().toStdString() << " to "
              << dst_info.absoluteFilePath().toStdString() << std::endl;
    return emit finished(1);
  }
  std::cout.flush();
  return emit finished(0);
}
