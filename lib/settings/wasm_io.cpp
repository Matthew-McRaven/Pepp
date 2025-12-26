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
#include "wasm_io.hpp"
#include <QFileDialog>

WASMIO::WASMIO(QObject *parent) : QObject(parent) {}

void WASMIO::save(const QString &filename, const QString &data) {
  QFileInfo file(filename);
  QFileDialog::saveFileContent(data.toUtf8(), file.fileName());
}

void WASMIO::load(const QString &nameFilter) {
  auto ready = [this](const QString &fileName, const QByteArray &fileContent) {
    if (!fileName.isEmpty()) {
      QFileInfo fileInfo(fileName);
      if (!QDir("/tmp/").exists()) QDir().mkdir("/tmp/");
      QString dest = "/tmp/" + fileInfo.fileName();
      _loadedName = "file:////tmp/" + fileInfo.fileName();
      QFile file(dest);
      if (!file.open(QIODevice::WriteOnly)) throw std::runtime_error("Could not open file for writing");
      file.write(fileContent);
      file.close();
      emit loaded();
    }
  };
  QFileDialog::getOpenFileContent(nameFilter, ready);
}


QString WASMIO::loadedName() const { return _loadedName; }
