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
#pragma once

#include <QObject>
#include <QtQmlIntegration>

class FileIO : public QObject {
  Q_OBJECT
  QML_NAMED_ELEMENT(FileIO)

public:
  FileIO(QObject *parent = nullptr);
  Q_INVOKABLE void save(const QString &filename, const QString &data);
  Q_INVOKABLE void loadCodeViaDialog(const QString &filters);
#ifndef __EMSCRIPTEN__
  Q_INVOKABLE void loadCodeFromFile(const QString &name, int arch, int abs);

private:
  QByteArray load(const QString &fileName);
#endif

signals:
  Q_INVOKABLE void codeLoaded(const QString &fileName, const QString &fileContent, int arch, int abstraction);
};
