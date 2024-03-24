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

ObjectCodeModel::ObjectCodeModel(QObject *parent) : QAbstractTableModel(parent) { ensureNotEmpty(); }

int ObjectCodeModel::rowCount(const QModelIndex &parent) const { return _rows.size(); }

int ObjectCodeModel::columnCount(const QModelIndex &parent) const { return 2; }

struct DisplayVisitor {
  QVariant operator()(const quint8 val) const { return QString::number(val, 16); }
  QVariant operator()(const ObjectCodeModel::Empty &) const { return ""; }
  QVariant operator()(const ObjectCodeModel::ZZ &) const { return "ZZ"; }
};

QVariant ObjectCodeModel::data(const QModelIndex &index, int role) const {
  qDebug() << "Data requested for:" << index.row() << index.column() << "Role:" << role;
  if (_rows.size() < index.row())
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
  if (_rows.size() < index.row())
    return false;

  bool ok = false, changed = false;
  auto &row = _rows[index.row()];
  auto &item = _rows[index.row()].data[index.column()];

  switch (role) {

  case Qt::EditRole: // use the display role as the default "editing" text
    [[fallthrough]];
  case Qt::DisplayRole:
    if (!value.canConvert<QString>())
      return false;
    else if (auto v = value.toString().toUShort(&ok, 16); ok) {
      // Discard values that don't pack into a byte.
      if (255 < v)
        return false;
      changed = !(std::holds_alternative<quint8>(item) && std::get<quint8>(item) == v);
      row.lastSet = index.column();
      item = static_cast<quint8>(v);
    } else if (value.toString().trimmed().isEmpty()) {
      // Disallow clearing a byte in the middle of a line.
      if (row.lastSet && *row.lastSet != index.column())
        return false;
      // Disallow clearing a byte in a line if the next line is non-empty.
      else if (index.row() < rowCount() - 1 && !isEmpty(index.row() + 1))
        return false;
      changed = !std::holds_alternative<Empty>(item);
      row.lastSet = (index.column() == 0) ? std::nullopt : std::optional{index.column() - 1};
      item = Empty{};
    } else
      return false;

    if (changed)
      emit dataChanged(index, index);
    // Must insert additional row if the last column is edited. Convert from 0- to 1-indexed.
    // Can only insert/delete rows if at EoL
    if (index.column() == columnCount() - 1) {
      // Row needs to be inserted if EoL is a non-space and we are on last line.
      if (row.lastSet.value_or(0) == index.column() && _rows.size() - 1 == index.row())
        insertRow(_rows.size());
      // Row needs to be removed if EoL is a space, we are on the second to last line, and last line is empty.
      else if (row.lastSet.value_or(0) != index.column() && _rows.size() - 2 == index.row() && isEmpty(rowCount() - 1))
        removeRow(_rows.size() - 1);
    }
    return true;
  default:
    return false;
  }
}

Qt::ItemFlags ObjectCodeModel::flags(const QModelIndex &index) const {
  if (_rows.size() < index.row())
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
  } else if (_rows.size() <= row) {
    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i)
      _rows.append(Row{.lastSet = std::nullopt, .data = QList<T>(columnCount())});
    endInsertRows();
    return true;
  } else
    return false;
}

bool ObjectCodeModel::removeRows(int row, int count, const QModelIndex &parent) {
  // Reject changes with OOB first row, or that delete more elements than in the table.
  if (row > 0 && (row + count - 1) < _rows.size() && count > 0) {
    beginRemoveRows(parent, row, row + count - 1);
    _rows.erase(_rows.begin() + row, _rows.begin() + row + count);
    endRemoveRows();
    return true;
  }
  return false;
}

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
  ensureNotEmpty();
  endResetModel();
}

void ObjectCodeModel::ensureNotEmpty() { _rows.prepend(Row{.lastSet = std::nullopt, .data = QList<T>(columnCount())}); }

bool ObjectCodeModel::isEmpty(int row) const {
  if (row >= _rows.size())
    return true;
  return !_rows[row].lastSet;
}
