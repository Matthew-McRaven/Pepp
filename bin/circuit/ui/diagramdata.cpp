#include <QVariant>

#include "diagramdata.hpp"
#include "diagramproperty.hpp"

DiagramData::DiagramData() {}

bool DiagramData::empty() const { return _cellData.empty(); }

const DiagramProperties *DiagramData::getDiagramProps(const PeppId id) const {
  auto data = _cells.find(id);
  if (data == _cells.end()) return nullptr;

  return data->second;
}

const DiagramProperties *DiagramData::getDiagramProps(const PeppPt &pt) const {
  auto id = _diagram_map.at(pt);

  if (!id.has_value()) return nullptr;

  return getDiagramProps(id.value());
}

DiagramProperties *DiagramData::getDiagramProps(const PeppId id) {
  auto data = _cells.find(id);
  if (data == _cells.end()) return nullptr;

  return data->second;
}

DiagramProperties *DiagramData::getDiagramProps(const PeppPt &pt) {
  auto id = _diagram_map.at(pt);

  if (!id.has_value()) return nullptr;

  return getDiagramProps(id.value());
}

DiagramProperties *DiagramData::createDiagramProps(const PeppKey &key) {
  //  See if something already exists at this location
  DiagramProperties *cell = getDiagramProps(key.top_left());
  if (cell != nullptr) return cell;

  //  Doesn't exist, create now
  _cellData.push_back(std::make_unique<DiagramProperties>());
  auto &data = _cellData.back();
  cell = static_cast<DiagramProperties *>(data.get());
  cell->setKey(key);

  //  point and size to rectangle
  auto id = _diagram_map.try_add(key);
  if (!id.has_value()) return nullptr;

  //  Save ID to diagram
  cell->setId(id.value());

  //  Insert into lookup table using id
  _cells.insert({id.value(), cell});

  return cell;
}

const LineProperties *DiagramData::getLineProps(const PeppKey &key) const {
  auto id = _line_map.at(key);

  if (!id.has_value()) return nullptr;

  auto data = _lines.find(id.value());
  if (data == _lines.end()) return nullptr;

  return data->second;
}

LineProperties *DiagramData::getLineProps(const PeppKey &key) {
  auto id = _line_map.at(key);

  if (!id.has_value()) return nullptr;

  auto data = _lines.find(id.value());
  if (data == _lines.end()) return nullptr;

  return data->second;
}

LineProperties *DiagramData::createLineProps(const PeppKey &key) {
  //  See if something already exists at this location
  LineProperties *line = getLineProps(key);
  if (line != nullptr) return line;

  //  Doesn't exist, create now
  _lineData.push_back(std::make_unique<LineProperties>());
  auto &data = _lineData.back();
  line = static_cast<LineProperties *>(data.get());
  line->setKey(key);

  //  point and size to rectangle
  auto id = _line_map.try_add(key);
  if (!id.has_value()) return nullptr;

  //  Save ID to diagram
  line->setId(id.value());

  //  Insert into lookup table using id
  _lines.insert({id.value(), line});

  return line;
}

bool DiagramData::clearDiagramData(const PeppKey &key) {
  auto id = _diagram_map.remove(key);
  if (!id.has_value()) return false;

  _cells.erase(id.value());

  // Erase matching id
  for (auto it = _cellData.begin(); it != _cellData.end();) {
    if ((*it)->id() == id.value()) it = _cellData.erase(it);
    else ++it;
  }

  return true;
}

bool DiagramData::moveData(const PeppPt &oldLocation, const PeppPt &newLocation) {
  auto id = _diagram_map.at(oldLocation);

  //  Nothing located at old location, just return
  if (!id.has_value()) return false;

  //  Only diagrams can be moved
  auto *data = getDiagramProps(id.value());
  if (data == nullptr) return false;

  if (_diagram_map.can_move_absolute(id.value(), newLocation)) {
      if (_diagram_map.move_absolute(id.value(), newLocation)) {
        //  Save key in cell for later lookups

        data->setKey({newLocation, data->key().size()});

        return true;
      }
  }
  return false;
}

bool DiagramData::canMoveData(const PeppId id, const PeppPt &newLocation) const {
  return _diagram_map.can_move_absolute(id, newLocation);
}

bool DiagramData::rotateData(const PeppId id) {
  auto box = _diagram_map.bounding_box(id);
  qDebug() << "box x" << box.x().lower() << "y" << box.y().lower() << "height" << box.height() << "width"
           << box.width();
  //  Only diagrams can be rotated
  auto *data = getDiagramProps(id);
  if (data == nullptr) return false;

  //  Temporary shim to keep diagram in same place during rotation
  //  Without shim, upper left changes, which will move diagram to
  //  different screen location
  i16 newX = data->key().top();
  i16 newY = data->key().left();
  PeppPt adjustedKey(newX, newY);
  qDebug() << "adj key x" << adjustedKey.x() << "y" << adjustedKey.y();
  //<< "height" << key.height() << "width" << key.width();

  //  move_relative below moves image in addition to rotate in place.
  //  if (_diagram_map.can_move_relative(id, {0, 0}, true)) {
  //    if (_diagram_map.move_relative(id, {0, 0}, true)) {
  if (_diagram_map.can_move_absolute(id, adjustedKey, true)) {
    if (_diagram_map.move_absolute(id, adjustedKey, true)) {
      //  Save key in cell for later lookups

      PeppRect newLocation = _diagram_map.bounding_box(id);
      qDebug() << "new box" << newLocation.x().lower() << "y" << newLocation.y().lower() << "height"
               << newLocation.height() << "width" << newLocation.width();

      data->setKey(newLocation);

      return true;
    }
  }
  return false;
}

bool DiagramData::rotateMoveData(const PeppId id) const { return _diagram_map.can_move_relative(id, {0, 0}, true); }