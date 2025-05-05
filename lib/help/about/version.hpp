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

#pragma once
#include <QtCore>
#include <QtQmlIntegration>

namespace about {
const char *const g_GIT_SHA1();
const char *const g_GIT_TAG();
int g_MAJOR_VERSION();
int g_MINOR_VERSION();
int g_PATCH_VERSION();
bool g_GIT_LOCAL_CHANGES();

class Version : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString git_sha READ git_sha CONSTANT)
  Q_PROPERTY(QString git_tag READ git_tag CONSTANT)
  Q_PROPERTY(bool git_dirty READ git_dirty CONSTANT)
  Q_PROPERTY(int version_major READ version_major CONSTANT)
  Q_PROPERTY(int version_minor READ version_minor CONSTANT)
  Q_PROPERTY(int version_patch READ version_patch CONSTANT)
  Q_PROPERTY(QString version_str_full READ version_str_full CONSTANT)
  QML_ELEMENT
  QML_SINGLETON

public:
  explicit Version(QObject *parent = nullptr);
  ~Version() override = default;
  static QString git_sha();
  static QString git_tag();
  static bool git_dirty();
  static int version_major();
  static int version_minor();
  static int version_patch();
  static QString version_str_full();
};

} // namespace about
