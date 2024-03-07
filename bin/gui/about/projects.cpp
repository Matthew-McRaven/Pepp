/*
 * Copyright (c) 2024 J. Stanley Warford, Matthew McRaven
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

#include "projects.hpp"

ProjectRoles *ProjectRoles::instance() {
  static ProjectRoles *_instance = new ProjectRoles;
  return _instance;
}

Projects::Projects(QObject *parent) : QAbstractListModel(parent), _deps(about::dependencies()) {}

int Projects::rowCount(const QModelIndex &parent) const { return _deps.size(); }

QVariant Projects::data(const QModelIndex &index, int role) const {
  using enum ProjectRoles::RoleNames;
  int row = index.row();
  if (!index.isValid() || row < 0 || row >= _deps.size())
    return QVariant();
  auto &item = _deps.at(row);
  switch (role) {
  case Name:
    return item.name;
  case URL:
    return item.url;
  case LicenseName:
    return item.licenseName;
  case LicenseSPDXID:
    return item.licenseSPDXID;
  case LicenseText:
    return item.licenseText;
  case DevDependency:
    return item.devDependency;
  default:
    return QVariant();
  }
}

QHash<int, QByteArray> Projects::roleNames() const {
  using enum ProjectRoles::RoleNames;
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
