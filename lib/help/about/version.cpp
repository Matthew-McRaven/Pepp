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

#include "./version.hpp"
#include <QClipboard>
#include <QGuiApplication>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QStringLiteral>

about::Version::Version(QObject *parent) : QObject(parent) {}
QString about::Version::git_sha() { return about::g_GIT_SHA1(); }
QString about::Version::git_tag() { return about::g_GIT_TAG(); }
bool about::Version::git_dirty() { return about::g_GIT_LOCAL_CHANGES(); }

QString about::Version::git_describe_short() {
  if (git_dirty()) return QStringLiteral("%1-dirty").arg(git_sha().first(8));
  else if (g_GIT_HAS_TAG()) return git_tag();
  else return git_sha().first(8);
}
int about::Version::version_major() { return about::g_MAJOR_VERSION(); }
int about::Version::version_minor() { return about::g_MINOR_VERSION(); }
int about::Version::version_patch() { return about::g_PATCH_VERSION(); }

QString _cxx_compiler() {
  static const auto ret = QStringLiteral("%1 %2").arg(about::g_CXX_COMPILER_ID()).arg(about::g_CXX_COMPILER_VERSION());
  return ret;
}
QString _build_system() {
  static const auto ret = QStringLiteral("%1 %2 %3")
                              .arg(about::g_BUILD_SYSTEM_NAME())
                              .arg(about::g_BUILD_SYSTEM_VERSION())
                              .arg(about::g_BUILD_SYSTEM_PROCESSOR());
  return ret;
}
QString _target_platform() {
  static const auto ret = QStringLiteral("%1").arg(QSysInfo::prettyProductName());
  return ret;
}
QString _target_abi() {
  static const auto ret = QStringLiteral("%1").arg(QSysInfo::buildAbi());
  return ret;
}
QStringList about::diagnostics() {
  using namespace Qt::StringLiterals;
  static const QString l1 = u"Based on commit %1 using Qt %2"_s.arg(g_GIT_SHA1(), QLibraryInfo::version().toString());
  static const QString l2 = u"Built on %1"_s.arg(g_BUILD_TIMESTAMP());
  static const QString l3 = u"Built by %1 using %2"_s.arg(_build_system(), _cxx_compiler());
  static const QString l4 = u"Running on %1 under %2"_s.arg(_target_platform(), _target_abi());
  static const auto ret = QStringList{l1, l2, l3, l4};
  return ret;
}

QString about::Version::diagnostic_str() {
  using namespace Qt::StringLiterals;
  return diagnostics().join("\n");
}

void about::Version::copy_diagnostics_to_clipboard() {
  // Only attempt clipboard access if the application has a clipboard.
  QClipboard *clipboard = QGuiApplication::clipboard();
  if (clipboard) clipboard->setText(diagnostic_str());
}

QString about::Version::version_str_full() {
  using namespace Qt::StringLiterals;
  return u"%1.%2.%3"_s.arg(version_major()).arg(version_minor()).arg(version_patch());
}

QString about::Version::build_timestamp() {
  using namespace Qt::StringLiterals;
  return u"%1"_s.arg(about::g_BUILD_TIMESTAMP());
}

QString about::Version::build_year() {
  using namespace Qt::StringLiterals;
  return u"%1"_s.arg(about::g_BUILD_YEAR());
}

QString about::Version::qt_version() { return QLibraryInfo::version().toString(); }
QString about::Version::qt_debug() { return QLibraryInfo::isDebugBuild() ? "true" : "false"; }
QString about::Version::qt_shared() { return QLibraryInfo::isSharedBuild() ? "true" : "false"; }

QString about::Version::cxx_compiler() { return _cxx_compiler(); }

QString about::Version::build_system() { return _build_system(); }

QString about::Version::target_platform() { return _target_platform(); }

QString about::Version::target_abi() { return _target_abi(); }

QString about::Version::target_graphics_api() {
  switch (QQuickWindow::graphicsApi()) {
  case QSGRendererInterface::Unknown: return QStringLiteral("Unknown");
  case QSGRendererInterface::Software: return QStringLiteral("Software");
  case QSGRendererInterface::OpenVG: return QStringLiteral("OpenVG");
  case QSGRendererInterface::OpenGL: return QStringLiteral("OpenGL");
  case QSGRendererInterface::Direct3D11: return QStringLiteral("Direct3D 11");
  case QSGRendererInterface::Vulkan: return QStringLiteral("Vulkan");
  case QSGRendererInterface::Metal: return QStringLiteral("Metal");
  case QSGRendererInterface::Null: return QStringLiteral("Null");
  case QSGRendererInterface::Direct3D12: return QStringLiteral("Direct3D 12");
  }
  return QStringLiteral("Unknown");
}

QString about::Version::target_qt_platform() { return QGuiApplication::platformName(); }
