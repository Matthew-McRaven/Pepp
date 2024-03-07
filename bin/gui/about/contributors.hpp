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
