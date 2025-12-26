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

#pragma once
#include <QtCore>
#include <QtQmlIntegration>

namespace about {
const char *const g_GIT_SHA1();
const char *const g_GIT_TAG();
const char *g_CXX_COMPILER_ID();
const char *g_CXX_COMPILER_VERSION();
const char *g_BUILD_SYSTEM_NAME();
const char *g_BUILD_SYSTEM_VERSION();
const char *g_BUILD_SYSTEM_PROCESSOR();
const char *g_BUILD_TIMESTAMP();
int g_MAJOR_VERSION();
int g_MINOR_VERSION();
int g_PATCH_VERSION();
bool g_GIT_LOCAL_CHANGES();

QStringList diagnostics();

class Version : public QObject {
  Q_OBJECT
  // Full diagnostic string, meant for ease of copying.
  Q_PROPERTY(QString diagnostic_str READ diagnostic_str CONSTANT)
  // Properties of pepp
  Q_PROPERTY(QString git_sha READ git_sha CONSTANT)
  Q_PROPERTY(QString git_tag READ git_tag CONSTANT)
  Q_PROPERTY(bool git_dirty READ git_dirty CONSTANT)
  Q_PROPERTY(int version_major READ version_major CONSTANT)
  Q_PROPERTY(int version_minor READ version_minor CONSTANT)
  Q_PROPERTY(int version_patch READ version_patch CONSTANT)
  Q_PROPERTY(QString version_str_full READ version_str_full CONSTANT)
  Q_PROPERTY(QString build_timestamp READ build_timestamp CONSTANT)
  // Properties of the machine running the application
  Q_PROPERTY(QString target_platform READ target_platform CONSTANT)
  Q_PROPERTY(QString target_abi READ target_abi CONSTANT)
  // Properties of our dependencies
  Q_PROPERTY(QString qt_version READ qt_version CONSTANT)
  Q_PROPERTY(QString qt_debug READ qt_debug CONSTANT)
  Q_PROPERTY(QString qt_shared READ qt_shared CONSTANT)
  // Properties of our build system
  Q_PROPERTY(QString cxx_compiler READ cxx_compiler CONSTANT)
  Q_PROPERTY(QString build_system READ build_system CONSTANT)
  QML_ELEMENT
  QML_SINGLETON

public:
  explicit Version(QObject *parent = nullptr);
  ~Version() override = default;
  static QString diagnostic_str();
  Q_INVOKABLE static void copy_diagnostics_to_clipboard();

  static QString git_sha();
  static QString git_tag();
  static bool git_dirty();
  static int version_major();
  static int version_minor();
  static int version_patch();
  static QString version_str_full();
  static QString build_timestamp();
  static QString target_platform();
  static QString target_abi();

  static QString qt_version();
  static QString qt_debug();
  static QString qt_shared();

  static QString cxx_compiler();
  static QString build_system();
};

} // namespace about
