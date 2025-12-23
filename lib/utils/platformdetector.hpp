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

class PlatformDetector : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool isWindows READ isWindows CONSTANT)
  Q_PROPERTY(bool isMac READ isMac CONSTANT)
  Q_PROPERTY(bool isLinux READ isLinux CONSTANT)
  Q_PROPERTY(bool isWASM READ isWASM CONSTANT)
  QML_NAMED_ELEMENT(PlatformDetector)
  QML_SINGLETON

public:
  explicit PlatformDetector(QObject *parent = nullptr);
  Q_INVOKABLE bool isWindows() const;
  Q_INVOKABLE bool isMac() const;
  Q_INVOKABLE bool isLinux() const;
  Q_INVOKABLE bool isWASM() const;
};
