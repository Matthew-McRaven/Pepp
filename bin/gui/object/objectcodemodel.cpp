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

#include "objectcodemodel.hpp"

ObjectCodeModel::ObjectCodeModel(QObject *parent) : QAbstractTableModel(parent) {}

int ObjectCodeModel::rowCount(const QModelIndex &parent) const {
  const auto cols = columnCount();
  if (_rows.size() == 0)
    return 1;
  // If a row is complete, will round up to the next row
  const auto &lastRow = _rows.last();
  if (std::holds_alternative<Empty>(lastRow.data.last()))
    return _rows.size();
  else
    return _rows.size() + 1;
}

int ObjectCodeModel::columnCount(const QModelIndex &parent) const { return 4; }

struct DisplayVisitor {
  QVariant operator()(const quint8 val) const { return QString::number(val, 16); }
  QVariant operator()(const ObjectCodeModel::Empty &) const { return ""; }
  QVariant operator()(const ObjectCodeModel::ZZ &) const { return "ZZ"; }
};

QVariant ObjectCodeModel::data(const QModelIndex &index, int role) const {
  qDebug() << "Data requested for:" << index.row() << index.column() << "Role:" << role;
  // FIXME: Remove placeholder in first column past the end of the data.
  if (_rows.size() == index.row()) {
    if (index.column() == 0)
      return "..";
    return {};
  } else if (_rows.size() < index.row())
    return {};

  switch (role) {
  case Qt::DisplayRole:
    return std::visit(DisplayVisitor{}, _rows[index.row()].data[index.column()]);
  default:
    return {};
  }
}

bool ObjectCodeModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  qDebug() << "Data set on:" << index.row() << index.column() << "Role:" << role;
  // Append a row if the first column past the end of the data is being edited.
  // Otherwise, reject edits beyond the size of the data.
  if (_rows.size() == index.row()) {
    // Replace fake row with a real one. Can't insertRows because rowCount remains the same.
    // Don't need to emit a dataChanged(...), since only the leading (i.e., current) cell is changed.
    if (index.column() == 0)
      _rows.append(Row{.lastSet = std::nullopt, .data = QList<T>(columnCount())});
    else
      return false;
  } else if (_rows.size() < index.row())
    return false;

  bool ok = false;
  auto &row = _rows[index.row()];
  auto &item = _rows[index.row()].data[index.column()];

  switch (role) {
  case Qt::DisplayRole:
    [[fallthrough]];
  case Qt::EditRole:
    if (value.canConvert<QString>()) {
      if (auto v = value.toString().toUShort(&ok, 16); ok) {
        if (std::holds_alternative<quint8>(item) && std::get<quint8>(item) == v)
          return true;
        else if (255 < v)
          return false;
        row.lastSet = index.column();
        item = static_cast<quint8>(v);
        // Must insert additional row if the last column is edited. Convert from 0- to 1-indexed.
        if (index.column() == (columnCount() - 1)) {
          beginInsertRows(QModelIndex(), index.row() + 1, index.row() + 1);
          endInsertRows();
        }
      } else if (value.toString().trimmed().isEmpty()) {
        if (std::holds_alternative<Empty>(item))
          return true;
        else if (row.lastSet && *row.lastSet != index.column())
          return false;
        row.lastSet = (index.column() == 0) ? std::nullopt : std::optional{index.column() - 1};
        item = Empty{};
      } else
        return false;
    } else
      return false;
    emit dataChanged(index, index);
    return true;
  default:
    return false;
  }
}

Qt::ItemFlags ObjectCodeModel::flags(const QModelIndex &index) const {
  if (_rows.size() == index.row()) {
    if (index.column() == 0)
      return Qt::ItemIsEditable | Qt::ItemIsEnabled;
    else
      return Qt::NoItemFlags;
  } else if (_rows.size() < index.row())
    return Qt::NoItemFlags;

  const auto &row = _rows[index.row()];
  if (index.column() == 0) {
  } else if (!row.lastSet || *row.lastSet + 1 < index.column())
    return Qt::NoItemFlags;
  return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool ObjectCodeModel::insertRows(int row, int count, const QModelIndex &parent) {
  if (row <= 0) {
    beginInsertRows(parent, 0, count - 1);
    for (int i = 0; i < count; ++i)
      _rows.prepend(Row{.lastSet = std::nullopt, .data = QList<T>(columnCount())});
    endInsertRows();
    return true;
  } else if (rowCount() <= row) {
    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i)
      _rows.append(Row{.lastSet = std::nullopt, .data = QList<T>(columnCount())});
    endInsertRows();
    return true;
  } else
    return false;
}

bool ObjectCodeModel::removeRows(int row, int count, const QModelIndex &parent) { return false; }

QModelIndex ObjectCodeModel::index(int row, int column, const QModelIndex &parent) const {
  if (row < 0 || column < 0 || row >= rowCount() || column >= columnCount())
    return {};
  return createIndex(row, column);
}

const QList<quint8> ObjectCodeModel::bytes() const { return {}; }

bool ObjectCodeModel::fromBytes(QList<quint8> bytes) { return true; }

void ObjectCodeModel::clear() {
  beginResetModel();
  _rows.clear();
  _terminalIndex = this->index(0, 0);
  endResetModel();
}
