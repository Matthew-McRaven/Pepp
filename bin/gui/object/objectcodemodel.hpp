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
#include <QAbstractTableModel>

// Forward-declare visitors so they can be friended
struct DisplayVisitor;

class ObjectCodeModel : public QAbstractTableModel {
  Q_OBJECT
public:
  explicit ObjectCodeModel(QObject *parent = nullptr);
  ~ObjectCodeModel() = default;

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
  bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
  QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;

  Q_INVOKABLE const QList<quint8> bytes() const;
public slots:
  Q_INVOKABLE bool fromBytes(QList<quint8> bytes);
  Q_INVOKABLE void clear();

private:
  friend DisplayVisitor;
  struct Empty {};
  struct ZZ {};
  using T = std::variant<Empty, ZZ, quint8>;
  struct Row {
    std::optional<quint16> lastSet = 0;
    QList<T> data;
  };
  static_assert(sizeof(T) <= 8, "T is too big");
  QList<Row> _rows;
  QPersistentModelIndex _terminalIndex = {};
};
