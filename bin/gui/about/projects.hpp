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

#pragma once

#include <QtCore>
#include "help/about/dependencies.hpp"

class ProjectRoles : public QObject {
  Q_OBJECT
public:
  enum RoleNames {
    Name = Qt::UserRole,
    URL = Qt::UserRole + 1,
    LicenseName = Qt::UserRole + 2,
    LicenseSPDXID = Qt::UserRole + 3,
    LicenseText = Qt::UserRole + 4,
    DevDependency = Qt::UserRole + 5
  };
  Q_ENUM(RoleNames)
  static ProjectRoles *instance();
  // Prevent copying and assignment
  ProjectRoles(const ProjectRoles &) = delete;
  ProjectRoles &operator=(const ProjectRoles &) = delete;

private:
  ProjectRoles() : QObject(nullptr) {}
};

class Projects : public QAbstractListModel {
  Q_OBJECT
public:
  explicit Projects(QObject *parent = nullptr);
  ~Projects() override = default;
  int rowCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

private:
  QList<about::Dependency> _deps;
};
