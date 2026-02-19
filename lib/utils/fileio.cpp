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
#include "fileio.hpp"
#include <QtWidgets/qfiledialog.h>
#include "constants.hpp"
#include "settings/settings.hpp"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

FileIO::FileIO(QObject *parent) : QObject(parent) {}

void FileIO::save(const QString &filename, const QString &data) {
  static const auto uri_start = QRegularExpression("file://");
  auto modified = filename;
  modified.replace(uri_start, "");
  QFile file(modified);
  if (file.open(QIODevice::WriteOnly)) {
    file.write(data.toUtf8());
    file.close();
  } else qWarning() << "Could not open file for writing";
#ifdef __EMSCRIPTEN__
  EM_ASM(FS.syncfs(function(){}););
#endif
}

void FileIO::loadCodeViaDialog(const QString &filters) {
  // arch, abstraction
  using enum pepp::Architecture;
  using enum pepp::Abstraction;
  using M = QMap<QString, std::pair<int, int>>; // Type alias to make formatting more legible.
  static const M filters3 = {{"Pep/10, ISA3 (*.pepo)", {(int)PEP10, (int)ISA3}},
                             {"Pep/9, ISA3 (*.pepo)", {(int)PEP9, (int)ISA3}},
                             {"Pep/10, ASMB3 (*.pep)", {(int)PEP10, (int)ASMB3}},
                             {"Pep/10, ASMB5 (*.pep)", {(int)PEP10, (int)ASMB5}},
                             {"Pep/9, ASMB5 (*.pep)", {(int)PEP9, (int)ASMB5}},
                             {"Text files (*.txt)", {0, 0}},
                             {"Any files (*.*)", {0, 0}}};
  static const QStringList filters2 = filters3.keys();
#ifdef __EMSCRIPTEN__
  auto ready = [this](const QString &fileName, const QByteArray &fileContent) {
    emit codeLoaded(fileName, fileContent, 0, 0);
  };
  QFileDialog::getOpenFileContent(filters2.join(";;"), ready);
#else
  QString selectedFilter;
  auto settings = pepp::settings::detail::AppSettingsData::getInstance();
  auto recents = settings->general()->recentFiles();
  QString startDir = "";
  if (!recents.empty()) {
    QFileInfo fi(recents.first().path());
    startDir = fi.absoluteDir().absolutePath();
  }
  auto fileName =
      QFileDialog::getOpenFileName(nullptr, "Open Source Code", startDir, filters2.join(";;"), &selectedFilter);
  if (fileName.isEmpty()) return;
  QByteArray ret = load(fileName);
  auto [arch, abs] = filters3.value(selectedFilter, {0, 0});
  emit codeLoaded(fileName, ret, arch, abs);
#endif
}

#ifndef __EMSCRIPTEN__
void FileIO::loadCodeFromFile(const QString &name, int arch, int abs) {
  auto ret = load(name);
  if (ret.isEmpty()) return;
  emit codeLoaded(name, ret, arch, abs);
}
QByteArray FileIO::load(const QString &fileName) {
  QFile file(fileName);
  QByteArray ret;
  if (file.size() > 1'000'000) {
    qWarning() << "File size exceeds 1MB. Will not load.";
  } else if (file.open(QIODevice::ReadOnly)) {
    ret = file.readAll();
    file.close();
  } else qWarning() << "Could not open file for reading";
  return ret;
}
#endif
