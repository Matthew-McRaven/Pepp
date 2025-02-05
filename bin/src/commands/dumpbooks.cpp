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

DumpBooksTask::DumpBooksTask(QObject *parent) : Task(parent) {}

void DumpBooksTask::run() {
  using namespace Qt::StringLiterals;
  auto base_output_path = QDir(QDir::currentPath());
  auto base_input_path = QDir(builtins::book_path);
  QDirIterator i(base_input_path, QDirIterator::Subdirectories);
  while (i.hasNext()) {
    auto f = QFileInfo(i.next());
    auto relative_input = base_input_path.relativeFilePath(f.absoluteFilePath());
    auto absolute_output = base_output_path.filePath(relative_input);
    if (f.isDir()) {
      if (!base_output_path.mkdir(absolute_output))
        qWarning() << "Failed to create output directory: " << absolute_output;
    } else {
      if (!QFile::copy(f.absolutePath(), absolute_output))
        qWarning() << "Failed to create output file: " << absolute_output;
    }
  }

  return emit finished(0);
}
