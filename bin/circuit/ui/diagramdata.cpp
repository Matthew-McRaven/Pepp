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

const DiagramProperties *DiagramData::getDiagramProps(const PeppKey &key) const {
  auto id = _diagram_map.at(key);

  if (!id.has_value()) return nullptr;

  auto data = _cells.find(id.value());
  if (data == _cells.end()) return nullptr;

  return static_cast<const DiagramProperties *>(data->second);
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

DiagramProperties *DiagramData::getDiagramProps(const PeppKey &key) {
  auto id = _diagram_map.at(key);

  if (!id.has_value()) return nullptr;

  return getDiagramProps(id.value());
}

DiagramProperties *DiagramData::createDiagramProps(const PeppKey &key) {
  //  See if something already exists at this location
  DiagramProperties *cell = getDiagramProps(key);
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

bool DiagramData::moveData(const PeppKey &oldKey, const PeppKey &newKey) {
  // Q_ASSERT(newKey.left() > 1);
  // Q_ASSERT(newKey.top() > 1);

  auto id = _diagram_map.at(oldKey);
  //  Nothing located at old location, just return
  if (!id.has_value()) return false;

  //  Only diagrams can be moved
  auto *data = getDiagramProps(oldKey);
  if (data == nullptr) return false;

  if (_diagram_map.can_move_absolute(id.value(), newKey.top_left())) {
    if (_diagram_map.move_absolute(id.value(), newKey.top_left())) {
      //  Save key in cell for later lookups
      data->setKey(newKey);

      return true;
    }
  }

  //  Move failed
  return false;
}

bool DiagramData::canMoveData(const PeppId id, const PeppPt &newLocation) const {
  return _diagram_map.can_move_absolute(id, newLocation);
}

bool DiagramData::cacheData(const PeppId id) {
  if (auto search = _cells.find(id); search != _cells.end()) _cachedDiagram = search->second;
  else {
    //  Error with lookup
    _cachedDiagram = nullptr;
    return false;
  }

  //  Remove pointer to old id
  _cells.erase(id);

  //  Erase old spatial data
  _diagram_map.remove(id);

  //  Object saved, but lookup information deleted (for now).
  return true;
}

//  Save diagram with new address
bool DiagramData::commit(const PeppPt &newLocation) {
  if (_cachedDiagram == nullptr) return false;
  //  Move cell into new location
  PeppRect newRect(newLocation, _cachedDiagram->key().size());
  auto id = _diagram_map.try_add(newRect);
  if (!id.has_value()) return false;

  //  Save ID to diagram
  _cachedDiagram->setId(id.value());
  _cachedDiagram->setKey(newRect);

  //  Insert data at new key
  _cells.insert({id.value(), _cachedDiagram});

  //  Clean up for next drag/drop operation
  _cachedDiagram = nullptr;

  return true;
}

//  Move failed, add back to model at original location
bool DiagramData::rollback() {
  if (_cachedDiagram == nullptr) return false;
  //  Move cell into new location
  auto id = _diagram_map.try_add(_cachedDiagram->key());
  if (!id.has_value()) return false;

  //  Save ID to diagram
  _cachedDiagram->setId(id.value());

  //  Insert data at new key
  _cells.insert({id.value(), _cachedDiagram});

  //  Clean up for next drag/drop operation
  _cachedDiagram = nullptr;

  return true;
}