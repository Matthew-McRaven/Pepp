#include "componentpropertymodel.hpp"

#include "schematic/circuitproject.hpp"
#include "schematic/circuitschematic.hpp"
#include "schematic/orient.hpp"

ComponentPropertyModel::ComponentPropertyModel(QObject *parent) : QAbstractListModel(parent) {}

int ComponentPropertyModel::rowCount(const QModelIndex &parent) const {
  // For list models only the root node (an invalid parent) should return the list's size. For all
  // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
  if (parent.isValid()) return 0;

  // FIXME: Implement me!
  return 2;
}

QVariant ComponentPropertyModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) return {};
  if (_componentId.value == 0 || _project == nullptr) return {};
  const auto comp = _project->schematic()->component(_componentId);

  switch (role) {
  // case Role::Name: return QString::fromStdString(comp->properties->);
  case Role::Id: return _componentId.value;
  case Role::Direction: {
    switch (comp->direction()) {
    case Direction::Right: return "Right";
    case Direction::Up: return "Up";
    case Direction::Down: return "Down";
    default: return "Left";
    }
  }
  }

  return {};
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

QHash<int, QByteArray> ComponentPropertyModel::roleNames() const {
  return {{Role::Name, "name"}, {Role::Id, "id"}, {Role::Direction, "direction"}};
}

void ComponentPropertyModel::setProject(CircuitProject *project) {
  if (_project != project) {
    beginResetModel();
    _project = project;
    endResetModel();
    emit projectChanged();
  }
}

void ComponentPropertyModel::setComponentId(u32 componentId) {
  if (_componentId.value != componentId) {
    beginResetModel();
    _componentId.value = componentId;
    endResetModel();
    emit componentChanged();
  }
}

/*void ComponentPropertyModel::setComponent(Component *component) {
  if (_component != component) {
    beginResetModel();
    _component = component;
    endResetModel();
    emit componentChanged();
  }
}*/

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
