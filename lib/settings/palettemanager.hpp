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
#pragma once
#include <QObject>
#include <QtQmlIntegration>
#include "./constants.hpp"

namespace pepp::settings {
class Palette;
class PaletteManager : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int display MEMBER _display CONSTANT);
  Q_PROPERTY(Role path MEMBER _path CONSTANT);
  Q_PROPERTY(Role isSystem MEMBER _isSystem CONSTANT);
  QML_ELEMENT

public:
  enum class Role : int {
    PathRole = Qt::UserRole + 1,
    IsSystemRole,
  };
  explicit PaletteManager(QObject *parent = nullptr);
  int rowCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  bool setData(const QModelIndex &index, const QVariant &value, int role) override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  QHash<int, QByteArray> roleNames() const override;
  Q_INVOKABLE void reload();
  Q_INVOKABLE int copy(int row);
  Q_INVOKABLE int importTheme(QString path);
  Q_INVOKABLE void deleteTheme(int row);

private:
  struct Entry {
    QString name{}, path{};
    bool isSystem = false;
  };
  QVector<Entry> _palettes{};
  void loadFrom(QString directory);
  static const int _display = Qt::DisplayRole;
  static const Role _path = Role::PathRole;
  static const Role _isSystem = Role::IsSystemRole;
};

} // namespace pepp::settings
