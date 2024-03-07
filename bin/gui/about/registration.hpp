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

#include <QQmlApplicationEngine>
#include <QtCore>
#include "commands/gui.hpp"
#include "help/about/dependencies.hpp"

class Version : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString git_sha READ git_sha CONSTANT)
  Q_PROPERTY(QString git_tag READ git_tag CONSTANT)
  Q_PROPERTY(bool git_dirty READ git_dirty CONSTANT)
  Q_PROPERTY(int version_major READ version_major CONSTANT)
  Q_PROPERTY(int version_minor READ version_minor CONSTANT)
  Q_PROPERTY(int version_patch READ version_patch CONSTANT)
  Q_PROPERTY(QString version_str_full READ version_str_full CONSTANT)
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
class Maintainer : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString name READ name CONSTANT)
  Q_PROPERTY(QString email READ email CONSTANT)
public:
  Maintainer(QString name, QString email, QObject *parent = nullptr);
  ~Maintainer() override = default;
  QString name();
  QString email();

private:
  QString _name = {}, _email = {};
};
class MaintainerList : public QAbstractListModel {
public:
  enum { NAME = Qt::UserRole, EMAIL = Qt::UserRole + 1 };
  explicit MaintainerList(QList<Maintainer *> list, QObject *parent = nullptr);
  ~MaintainerList() override = default;
  int rowCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

private:
  QList<Maintainer *> _list;
};

class Contributors : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString text READ text CONSTANT)
public:
  explicit Contributors(QObject *parent = nullptr);
  ~Contributors() override = default;
  static QString text();
};

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

namespace about {
void registerTypes(QQmlApplicationEngine &engine);
}
