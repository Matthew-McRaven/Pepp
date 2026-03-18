#include "diagramdatamodel.hpp"

DiagramDataModel::DiagramDataModel(QObject *parent) : QAbstractTableModel(parent) {}

bool DiagramDataModel::move(const QModelIndex oldIndex, const QModelIndex newIndex) {
  if (!oldIndex.isValid() || !newIndex.isValid()) return false;

  //  If moving to same location, just return
  if (oldIndex == newIndex) return false;

  //  Create new key from scratch since this item does not exist yet.
  const auto oldKey = convertIndex(oldIndex);
  const auto newKey = convertIndex(newIndex);

  //  Cell is moving, pass old and new locations

  setCurrentIndex(newIndex);
  if (!_data.moveData(oldKey, newKey)) return false;

  //  Notify UI of change
  emit dataChanged(oldIndex, oldIndex);
  emit dataChanged(newIndex, newIndex);
  return true;
}

void DiagramDataModel::update(const QModelIndex &index) {
  if (!index.isValid()) return;

  emit dataChanged(index, index);
}

bool DiagramDataModel::clearItemData(const QModelIndexList &indexes) {
  bool ok = true;
  for (const QModelIndex &index : indexes) ok &= clearItemData(index);
  return ok;
}

bool DiagramDataModel::clearItemData(const QModelIndex &index) {
  if (!index.isValid()) return false;

  const auto *data = item(index);

  if (_data.clearDiagramData(data->key())) {
    emit dataChanged(index, index);
    return true;
  }
  return false;
}

const QModelIndex DiagramDataModel::currentIndex() const { return _current; }

void DiagramDataModel::setCurrentIndex(const QModelIndex v) {
  if (v != _current) {
    _current = v;
    emit currentIndexChanged();
  }
}

DiagramProperties *DiagramDataModel::item(const QModelIndex &index) {
  if (!index.isValid()) return nullptr;

  auto data = _data.getDiagramProps(convertIndex(index));
  return data;
}

DiagramProperties *DiagramDataModel::createItem(const QModelIndex &index) {
  if (!index.isValid()) return nullptr;

  auto data = _data.createDiagramProps(convertIndex(index));
  emit dataChanged(index, index);
  return data;
}

QModelIndex DiagramDataModel::index(int row, int column, const QModelIndex &parent) const {
  // Check if row and column are within bounds and parent is invalid
  if (!hasIndex(row, column, parent)) return {};

  // The internalPointer can store a pointer to your underlying data for quick access in data()
  // For a simple list, we can just use the row and column
  return createIndex(row, column, nullptr); // Use 0 or another pointer if you use internal data
}
int DiagramDataModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) return 0;

  return _rowSize;
}

int DiagramDataModel::columnCount(const QModelIndex &parent) const {
  if (parent.isValid()) return 0;

  return _colSize;
}

QVariant DiagramDataModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) return {};

  const auto *item = _data.getDiagramProps(convertIndex(index));
  return item->get(role);
}

bool DiagramDataModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (!index.isValid()) return false;

  //  Set currently selected if value is true
  if (role == DiagramProperty::Role::Selected && value.toBool()) {
    setCurrentIndex(index);
    emit dataChanged(index, index);
  }

  auto *item = _data.getDiagramProps(convertIndex(index));
  if (item == nullptr) return false;

  item->set(role, value);
  emit dataChanged(index, index);

  return true;
}

Qt::ItemFlags DiagramDataModel::flags(const QModelIndex &index) const {
  if (!index.isValid()) return Qt::NoItemFlags;

  return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
}

QHash<int, QByteArray> DiagramDataModel::roleNames() const {
  return {{DiagramProperty::Role::Name, "name"},
          {DiagramProperty::Role::Id, "id"},
          {DiagramProperty::Role::ImageSource, "imageSource"},
          {DiagramProperty::Role::Type, "diagramType"},
          {DiagramProperty::Role::InputNo, "inputNo"},
          {DiagramProperty::Role::OutputNo, "outputNo"}};
}
