#include <QVariant>

#include "diagramdata.hpp"
#include "diagramproperty.hpp"

DiagramData::DiagramData() {}

bool DiagramData::empty() const { return _cellData.empty(); }

const DiagramProperties *DiagramData::getDiagramProps(const PeppKey &key) const {
  auto id = _diagram_map.at(key);

  if (!id.has_value()) return nullptr;

  auto data = _cells.find(id.value());
  if (data == _cells.end()) return nullptr;

  return static_cast<const DiagramProperties *>(data->second);
}

DiagramProperties *DiagramData::getDiagramProps(const PeppKey &key) {
  auto id = _diagram_map.at(key);

  if (!id.has_value()) return nullptr;

  auto data = _cells.find(id.value());
  if (data == _cells.end()) return nullptr;

  // return data->second;
  return static_cast<DiagramProperties *>(data->second);
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

  // return data->second;
  return data->second;
}

LineProperties *DiagramData::createLineProps(const PeppKey &key) {
  //  See if something already exists at this location
  LineProperties *line = getLineProps(key);
  if (line != nullptr) return line;

  //  Doesn't exist, create now
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

  for (auto &it : _cellData) {
    if (it->id() == id.value()) _cellData.remove(it);
  }

  return true;
}

void DiagramData::moveData(const PeppKey &oldKey, const PeppKey &newKey) {
  Q_ASSERT(newKey.left() > 1);
  Q_ASSERT(newKey.top() > 1);

  auto id = _diagram_map.at(oldKey);
  //  Nothing located at old location, just return
  if (!id.has_value()) return;

  //  Only diagrams can be moved
  auto *data = getDiagramProps(oldKey);
  if (data == nullptr) return;

  //  Save key in cell for later lookups
  data->setKey(newKey);

  //  Remove pointer to old id
  _cells.erase(id.value());

  //  Erase old spacial data
  _diagram_map.remove(id.value());

  //  Move cell into new location
  id = _diagram_map.try_add(newKey);
  Q_ASSERT(id.has_value());

  //  Save ID to diagram
  data->setId(id.value());

  //  Insert data at new key
  _cells.insert({id.value(), data});
}
