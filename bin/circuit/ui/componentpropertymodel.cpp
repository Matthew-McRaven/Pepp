#include "componentpropertymodel.hpp"

ComponentPropertyModel::ComponentPropertyModel(QObject *parent) : QAbstractListModel(parent) {}

int ComponentPropertyModel::rowCount(const QModelIndex &parent) const {
  // For list models only the root node (an invalid parent) should return the list's size. For all
  // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
  if (parent.isValid()) return 0;

  // FIXME: Implement me!
}

QVariant ComponentPropertyModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) return QVariant();

  // FIXME: Implement me!
  return QVariant();
}

bool ComponentPropertyModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (data(index, role) != value) {
    // FIXME: Implement me!
    emit dataChanged(index, index, {role});
    return true;
  }
  return false;
}

Qt::ItemFlags ComponentPropertyModel::flags(const QModelIndex &index) const {
  if (!index.isValid()) return Qt::NoItemFlags;

  return QAbstractItemModel::flags(index) | Qt::ItemIsEditable; // FIXME: Implement me!
}

void ComponentPropertyModel::setProject(CircuitProject *project) {
  if (_project != project) {
    beginResetModel();
    _project = project;
    endResetModel();
    emit projectChanged();
  }
}

/*bool ComponentPropertyModel::insertRows(int row, int count, const QModelIndex &parent) {
  beginInsertRows(parent, row, row + count - 1);
  // FIXME: Implement me!
  endInsertRows();
  return true;
}

bool ComponentPropertyModel::removeRows(int row, int count, const QModelIndex &parent) {
  beginRemoveRows(parent, row, row + count - 1);
  // FIXME: Implement me!
  endRemoveRows();
  return true;
}*/
