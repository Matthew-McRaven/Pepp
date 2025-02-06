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

#include "dumpbooks.hpp"
#include <QDir>
#include <iostream>
#include "../shared.hpp"
#include "builtins/registry.hpp"

DumpBooksTask::DumpBooksTask(QString dir, QObject *parent) : Task(parent), _dir(dir) {}

void DumpBooksTask::run() {
  using namespace Qt::StringLiterals;
  auto base_output_path = QDir(_dir);
  if (!base_output_path.exists() && !QDir().mkpath(base_output_path.path())) {
    qWarning() << "Failed to create output directory: " << base_output_path.path();
    return emit finished(1);
  }
  auto base_input_path = QDir(builtins::default_book_path);
  QDirIterator i(base_input_path, QDirIterator::Subdirectories);
  while (i.hasNext()) {
    auto f = QFileInfo(i.next());
    auto relative_input = base_input_path.relativeFilePath(f.absoluteFilePath());
    auto file_input = QFile(f.absoluteFilePath());
    auto absolute_output = base_output_path.absoluteFilePath(relative_input);
    auto output_info = QFileInfo(absolute_output);
    if (f.isDir() && output_info.exists() && !output_info.isDir()) {
      if (!QFile::remove(output_info.absoluteFilePath())) {
        qWarning() << "Failed to remove conflicting output file: " << absolute_output;
        return emit finished(1);
      }
    }
    if (f.isDir()) {
      if (!QDir().mkdir(absolute_output)) {
        qWarning() << "Failed to create output directory: " << absolute_output;
        return emit finished(1);
      }
    } else {
      QFile::remove(absolute_output);
      qDebug() << f.absoluteFilePath() << absolute_output;
      if (!file_input.copy(absolute_output)) qWarning() << "Failed to create output file: " << absolute_output;
    }
  }

  return emit finished(0);
}
