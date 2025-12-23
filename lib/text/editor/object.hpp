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

class ObjectUtilities : public QObject {
  Q_OBJECT
  Q_PROPERTY(int bytesPerRow READ bytesPerRow WRITE setBytesPerRow NOTIFY bytesPerRowChanged)
  QML_NAMED_ELEMENT(ObjectUtilities)

public:
  explicit ObjectUtilities(QObject *parent = nullptr);
  Q_INVOKABLE static bool valid(int key);
  Q_INVOKABLE QString format(QString input, bool includeZZ = true) const;
  // Remove spaces and non space/hex chars
  Q_INVOKABLE QString normalize(QString input) const;
public slots:
  void setBytesPerRow(int bytes);
  int bytesPerRow() const;
signals:
  void bytesPerRowChanged();

private:
  int _bytesPerRow = 16;
};
