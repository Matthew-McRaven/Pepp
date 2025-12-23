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
#include <QtCore/qabstractitemmodel.h>
#include <qqmlintegration.h>
namespace builtins {
class Registry;
class Figure;
} // namespace builtins

class FavoriteFigureModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ _rowCount NOTIFY rowCountChanged)
  QML_ELEMENT
public:
  enum class Roles {
    FigurePtrRole = Qt::UserRole + 1,
    NameRole,
    TypeRole,
    DescriptionRole,
  };
  Q_ENUM(Roles);
  explicit FavoriteFigureModel(QObject *parent = nullptr);
  // Helper to expose rowCount as a property to QML.
  int _rowCount() const { return rowCount({}); }
  int rowCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;
signals:
  void rowCountChanged(int);

private:
  QSharedPointer<builtins::Registry> _registry;
  QList<QSharedPointer<const builtins::Figure>> _figures;
};
