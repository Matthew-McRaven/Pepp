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

#include "contributors.hpp"
#include "help/about/pepp.hpp"

Maintainer::Maintainer(QString name, QString email, QObject *parent) : QObject(parent), _name(name), _email(email) {}
QString Maintainer::name() { return _name; }
QString Maintainer::email() { return _email; }

MaintainerList::MaintainerList(QList<Maintainer *> list, QObject *parent) : QAbstractListModel(parent), _list(list) {
  // Must re-parent to avoid memory leaks.
  for (auto *item : list) item->setParent(this);
}

int MaintainerList::rowCount(const QModelIndex &parent) const { return _list.size(); }

QVariant MaintainerList::data(const QModelIndex &index, int role) const {
  int row = index.row();
  if (!index.isValid() || row < 0 || row >= _list.size()) return QVariant();
  auto *item = _list.at(row);
  switch (role) {
  case NAME: return item->name();
  case EMAIL: return item->email();
  case ITEM: return QVariant::fromValue(item);
  default: return QVariant();
  }
}

QHash<int, QByteArray> MaintainerList::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[NAME] = "name";
  roles[EMAIL] = "email";
  roles[ITEM] = "item";
  return roles;
}

Contributors::Contributors(QObject *parent) : QObject(parent) {}

QString Contributors::text() {
  static QString text = about::contributors().join(", ");
  return text;
}
