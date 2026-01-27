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
#include <QSortFilterProxyModel>
#include <QTransposeProxyModel>
#include <qqmlintegration.h>

class RowFilterModel : public QSortFilterProxyModel {
  Q_OBJECT
  Q_PROPERTY(int row READ row WRITE setRow NOTIFY rowChanged)
  QML_NAMED_ELEMENT(RowFilterModel);

public:
  RowFilterModel(QObject *parent = nullptr);
  int row() const;
  void setRow(int row);

signals:
  void rowChanged();

protected:
  bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
  int _row = -1;
};

struct ForeginQTransposeProxyModel {
  Q_GADGET
  QML_FOREIGN(QTransposeProxyModel);
  QML_NAMED_ELEMENT(TransposeProxyModel);
};
