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

#include "dumptex.hpp"
#include <QDir>
#include "help/builtins/figure.hpp"
#include "toolchain/helpers/assemblerregistry.hpp"

DumpTexTask::DumpTexTask(QString dir, QObject *parent) : Task(parent), _dir(dir) {}

void DumpTexTask::run() {
  using namespace Qt::StringLiterals;

  auto base_output_path = QDir(_dir);
  if (!base_output_path.exists()) {
    qWarning() << "Output directory does not exist: " << base_output_path.path();
    return emit finished(1);
  }
  auto books = helpers::registry_with_assemblers();
  auto book = helpers::book(6, &*books);
  if (book.isNull()) return emit finished(1);

  for (const auto &figure : book->figures()) {
    for (const auto &element : figure->typesafeFragments()) {
      if (element->exportPath.isEmpty()) continue;
      auto disk_path = base_output_path.absoluteFilePath(element->exportPath);
      auto contents = element->contents().trimmed() + "\n";
      QFile file(disk_path);
      if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for writing: " << disk_path;
        return emit finished(1);
      }
      qDebug().noquote() << "Writing to file: " << disk_path;
      QTextStream out(&file);
      out << contents;
      out.flush();
      file.close();
    }
  }

  return emit finished(0);
}
