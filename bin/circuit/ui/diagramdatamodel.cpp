#include "diagramdatamodel.hpp"
#include "graphiccanvas.hpp" //  GraphicCanvas::diagramGeometry

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

const QModelIndex DiagramDataModel::currentIndex() const { return _current; }

void DiagramDataModel::setCurrentIndex(const QModelIndex v) {
  if (v != _current) {
    _current = v;
    emit currentIndexChanged();
  }
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

  // return item->get(role);
  if (item == nullptr) return {};
  switch (role) {
  case DiagramDataModel::Role::Id: return item->id();
  case DiagramDataModel::Role::Name: return item->name();
  case DiagramDataModel::Role::ImageSource: return item->imageSource();
  case DiagramDataModel::Role::DiagramType: return item->type();
  case DiagramDataModel::Role::InputNo: return item->inputNo();
  case DiagramDataModel::Role::OutputNo: return item->outputNo();
  case DiagramDataModel::Role::Selected: return item->selected();
  case DiagramDataModel::Role::Orientation:
    return item->orientation();
  }
}

bool DiagramDataModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (!index.isValid()) return false;

  //  Set currently selected if value is true
  if (role == DiagramDataModel::Role::Selected && value.toBool()) {
    setCurrentIndex(index);
    emit dataChanged(index, index);
  }

  auto *item = _data.getDiagramProps(convertIndex(index));
  if (item == nullptr) return false;

  switch (role) {
  case DiagramDataModel::Role::Name: item->setName(value.toString()); break;
  case DiagramDataModel::Role::ImageSource: item->setImageSource(value.toString()); break;
  case DiagramDataModel::Role::DiagramType: item->setType(static_cast<DiagramType::Type>(value.toInt())); break;
  case DiagramDataModel::Role::InputNo: item->setInputNo(value.toInt()); break;
  case DiagramDataModel::Role::OutputNo: item->setOutputNo(value.toInt()); break;
  case DiagramDataModel::Role::Selected: item->setSelected(value.toBool()); break;
  case DiagramDataModel::Role::Orientation: {
    const int oldV = item->orientation();
    const int newV = value.toInt();

    item->setOrientation(newV);
    //  Was there a switch between vertical and horizontal?
    bool change = ((oldV + newV) % 180) != 0;
    if (change) {
      _data.rotateData(item->id());

      //  Rotate swaps height and width of collision mask
      //  We need to perform same transformation to screen size
      item->setGridRectangle(GraphicCanvas::diagramGeometry(item));
    }
    break;
  }
  }

  emit dataChanged(index, index);

  return true;
}

Qt::ItemFlags DiagramDataModel::flags(const QModelIndex &index) const {
  if (!index.isValid()) return Qt::NoItemFlags;

  return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
}

QHash<int, QByteArray> DiagramDataModel::roleNames() const {
  return {{DiagramDataModel::Role::Name, "name"},
          {DiagramDataModel::Role::Id, "id"},
          {DiagramDataModel::Role::ImageSource, "imageSource"},
          {DiagramDataModel::Role::DiagramType, "diagramType"},
          {DiagramDataModel::Role::InputNo, "inputNo"},
          {DiagramDataModel::Role::OutputNo, "outputNo"},
          {DiagramDataModel::Role::Selected, "selected"},
          {DiagramDataModel::Role::Orientation, "orientation"}};
}
