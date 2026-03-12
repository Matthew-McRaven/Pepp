#include <QVariant>

#include "diagramdata.hpp"
#include "diagramproperty.hpp"

DiagramData::DiagramData() {}

bool DiagramData::empty() const { return _data.empty(); }

const BaseProperties *DiagramData::getProps(const PeppKey &key) const {
  auto id = _spatial_map.at(key);

  if (!id.has_value()) return nullptr;

  auto data = _cells.find(id.value());
  if (data == _cells.end()) return nullptr;

  return data->second;
}

BaseProperties *DiagramData::getProps(const PeppKey &key) {
  auto id = _spatial_map.at(key);

  if (!id.has_value()) return nullptr;

  auto data = _cells.find(id.value());
  if (data == _cells.end()) return nullptr;

  // return data->second;
  return data->second;
}

DiagramProperties *DiagramData::createDiagramProps(const PeppKey &key) {
  //  See if something already exists at this location
  DiagramProperties *cell = static_cast<DiagramProperties *>(getProps(key));
  if (cell != nullptr) return cell;

  //  Doesn't exist, create now
  _data.push_back(std::make_unique<DiagramProperties>());
  auto &data = _data.back();
  cell = static_cast<DiagramProperties *>(data.get());
  cell->setKey(key);

  //  point and size to rectangle
  auto id = _spatial_map.try_add(key);
  if (!id.has_value()) return nullptr;

  //  Save ID to diagram
  cell->setId(id.value());

  //  Insert into lookup table using id
  _cells.insert({id.value(), cell});

  return cell;
}

LineProperties *DiagramData::createLineProps(const PeppKey &key) {
  //  See if something already exists at this location
  LineProperties *line = static_cast<LineProperties *>(getProps(key));
  if (line != nullptr) return line;

  //  Doesn't exist, create now
  _data.push_back(std::make_unique<LineProperties>());
  auto &data = _data.back();
  line = static_cast<LineProperties *>(data.get());
  line->setKey(key);

  //  point and size to rectangle
  auto id = _spatial_map.try_add(key);
  if (!id.has_value()) return nullptr;

  //  Save ID to diagram
  line->setId(id.value());

  //  Insert into lookup table using id
  _cells.insert({id.value(), line});

  return line;
}

bool DiagramData::clearData(const PeppKey &key) {
  auto id = _spatial_map.remove(key);
  if (!id.has_value()) return false;

  _cells.erase(id.value());

  for (const auto &it : _data) {
    if (it->id() == id.value()) _data.remove(it);
  }
  //  TODO: Need to remove from _data.
  // auto it =
  //    std::find_if(_data.cbegin(), _data.cend(), [id](const BaseProperties &data) { return data.id() == id.value();
  //    });

  // if (it != _data.cend()) _data.erase(it);

  return true;
}

void DiagramData::moveData(const PeppKey &oldKey, const PeppKey &newKey) {
  Q_ASSERT(newKey.left() > 1);
  Q_ASSERT(newKey.top() > 1);

  auto id = _spatial_map.at(oldKey);
  //  Nothing located at old location, just return
  if (!id.has_value()) return;

  auto *data = getProps(oldKey);
  if (data == nullptr) return;

  //  Save key in cell for later lookups
  data->setKey(newKey);

  //  Remove pointer to old id
  _cells.erase(id.value());

  //  Erase old spacial data
  _spatial_map.remove(id.value());

  //  Move cell into new location
  id = _spatial_map.try_add(newKey);
  Q_ASSERT(id.has_value());

  //  Save ID to diagram
  data->setId(id.value());

  //  Insert data at new key
  _cells.insert({id.value(), data});
}
