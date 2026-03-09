#include <QVariant>

#include "diagramdata.hpp"
#include "diagramproperty.hpp"

DiagramData::DiagramData() {}

bool DiagramData::empty() const { return _data.empty(); }

const DiagramProperties *DiagramData::getDiagramProps(const PeppKey &key) const {
  auto id = _spatial_map.at(key);

  if (!id.has_value()) return nullptr;

  auto data = _cells.find(id.value());
  if (data == _cells.end()) return nullptr;

  return data->second;
}

DiagramProperties *DiagramData::getDiagramProps(const PeppKey &key) {
  auto id = _spatial_map.at(key);

  if (!id.has_value()) return nullptr;

  auto data = _cells.find(id.value());
  if (data == _cells.end()) return nullptr;

  return data->second;
}

DiagramProperties *DiagramData::createDiagramProps(const PeppKey &key) {
  //  See if something already exists at this location
  DiagramProperties *cell = getDiagramProps(key);
  if (cell != nullptr) return cell;

  //  Doesn't exist, create now
  auto &data = _data.emplace_back();
  data.setKey(key);

  //  point and size to rectangle
  auto id = _spatial_map.try_add(key);

  _cells.insert({id.value(), &data});

  return &data;
}

bool DiagramData::clearData(const PeppKey &key) {
  auto id = _spatial_map.remove(key);
  if (!id.has_value()) return false;

  _cells.erase(id.value());

  //  TODO: Need to remove from _data.
  auto it = std::find_if(_data.cbegin(), _data.cend(),
                         [&id](const DiagramProperties &data) { return data.id() == id.value(); });
  return true;
}

void DiagramData::moveData(const PeppKey &oldKey, const PeppKey &newKey) {
  auto id = _spatial_map.at(oldKey);
  //  Nothing located at old location, just return
  if (!id.has_value()) return;

  auto *cell = getDiagramProps(oldKey);
  if (cell == nullptr) return;

  //  Save key in cell for later lookups
  cell->setKey(newKey);

  //  Remove pointer to old id
  _cells.erase(id.value());

  //  Erase old spacial data
  _spatial_map.remove(id.value());

  //  Move cell into new location
  id = _spatial_map.try_add(newKey);

  //  Insert data at new key
  _cells.insert({id.value(), cell});
}
