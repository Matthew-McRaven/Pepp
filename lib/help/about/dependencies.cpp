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
#include "./dependencies.hpp"
#include <QDebug>
#include "./read.hpp"

QList<about::Dependency> about::dependencies() {
  auto depText = detail::readFile(":/about/dependencies.csv");
  if (!depText.has_value()) return {};
  QList<about::Dependency> ret;
  bool first = true;
  for (auto &line : depText->split("\n")) {
    // First line is headers, skip.
    if (first) {
      first = false;
      continue;
    } else if (line.isEmpty()) continue;
    // There should be 6 headers: name, url, license name, license SPDX ID,
    // license text file, and a flag for if development dependency.
    auto parts = line.split(",");
    if (parts.size() != 6) {
      qWarning() << "Failed to parse dependency row: " << line << "\n";
      return {};
    }

    // Parse devDependency to flag. Any non-zero should set the flag
    bool flag;
    bool v = !(parts[5].toInt(&flag) == 0);
    if (!flag) qWarning() << "Failed to parse devDependency as int: " << parts[5] << "\n";

    auto lineText = detail::readFile(parts[4]);
    if (!lineText.has_value()) return {};
    ret.push_back(about::Dependency{.name = parts[0],
                                    .url = parts[1],
                                    .licenseName = parts[2],
                                    .licenseSPDXID = parts[3],
                                    .licenseText = *lineText,
                                    .devDependency = flag ? v : false});
  }
  return ret;
}

about::DependencyRoles *about::DependencyRoles::instance() {
  static DependencyRoles *_instance = new DependencyRoles;
  return _instance;
}

about::Dependencies::Dependencies(QObject *parent) : QAbstractListModel(parent), _deps(about::dependencies()) {}

int about::Dependencies::rowCount(const QModelIndex &parent) const { return _deps.size(); }

QVariant about::Dependencies::data(const QModelIndex &index, int role) const {
  using enum DependencyRoles::RoleNames;
  int row = index.row();
  if (!index.isValid() || row < 0 || row >= _deps.size()) return QVariant();
  auto &item = _deps.at(row);
  switch (role) {
  case Name: return item.name;
  case URL: return item.url;
  case LicenseName: return item.licenseName;
  case LicenseSPDXID: return item.licenseSPDXID;
  case LicenseText: return item.licenseText;
  case DevDependency: return item.devDependency;
  default: return QVariant();
  }
}

QHash<int, QByteArray> about::Dependencies::roleNames() const {
  using enum DependencyRoles::RoleNames;
  QHash<int, QByteArray> roles = {
      {Name, "name"},
      {URL, "url"},
      {LicenseName, "licenseName"},
      {LicenseSPDXID, "licenseSPDXID"},
      {LicenseText, "licenseText"},
      {DevDependency, "devDependency"},
  };
  return roles;
}
