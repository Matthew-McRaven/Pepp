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
#include "statusbitmodel.hpp"

Flag::Flag(QString name, std::function<bool()> value) : _name(name), _fn(value) {}

QString Flag::name() const { return _name; }

bool Flag::value() const { return _fn(); }

FlagModel::FlagModel(QObject *parent) : QAbstractListModel(parent) {}

int FlagModel::rowCount(const QModelIndex &) const { return _flags.size(); }

QVariant FlagModel::data(const QModelIndex &index, int role) const {
  const auto row = index.row();
  if (!index.isValid() || row < 0 || row >= _flags.size()) return {};
  auto flag = _flags[row];

  switch (role) {
  case Qt::DisplayRole: return flag->name();
  case static_cast<int>(Roles::Value): return flag->value();
  }
  return {};
}

void FlagModel::appendFlag(QSharedPointer<Flag> flag) {
  beginResetModel();
  _flags.append(flag);
  endResetModel();
}

void FlagModel::onUpdateGUI() {
  beginResetModel();
  endResetModel();
}

QHash<int, QByteArray> FlagModel::roleNames() const {
  static QHash<int, QByteArray> ret{{Qt::DisplayRole, "text"}, {(int)Roles::Value, "checked"}};
  return ret;
}
