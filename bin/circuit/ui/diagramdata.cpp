#include <QVariant>

#include "diagramdata.hpp"
#include "diagramproperty.hpp"

DiagramData::DiagramData() {}

bool DiagramData::empty() const { return _data.empty(); }

const DiagramProperties *DiagramData::getDiagramProps(const PeppPt &point) const {
  auto id = _spatial_map.at(PeppKey(point));

  if (!id.has_value()) return nullptr;

  auto data = _cells.find(id.value());
  if (data == _cells.end()) return nullptr;

  return data->second;
}

DiagramProperties *DiagramData::getDiagramProps(const PeppPt &point) {
  //  This function isn't working. Known items are not returned
  auto id = _spatial_map.at(PeppKey(point));

  if (!id.has_value()) return nullptr;

  auto data = _cells.find(id.value());
  if (data == _cells.end()) return nullptr;

  return data->second;
}

DiagramProperties *DiagramData::createDiagramProps(const PeppPt &point, const PeppSize &size) {
  //  See if something already exists at this location
  DiagramProperties *cell = getDiagramProps(point);
  if (cell != nullptr) return cell;

  //  Doesn't exist, create now
  auto &data = _data.emplace_back();

  //  point and size to rectangle
  PeppRect key{point, size};
  auto id = _spatial_map.try_add(key);

  _cells.insert({id.value(), &data});

  return &data;
}

bool DiagramData::clearData(const PeppPt &point) {
  // auto it = std::find_if(_keys.cbegin(), _keys.cend(),
  //                        [&key](const std::pair<quint32, PeppKey> &pair) { return pair.second == key; });
  // if (it == _keys.cend()) return false;
  //_keys.erase(it);
  auto id = _spatial_map.remove(point);
  if (!id.has_value()) return false;

  _cells.erase(id.value());

  //  TODO: Need to remove from _data.
  return true;
}

void DiagramData::moveData(const PeppPt &oldKey, const PeppPt &newKey) {
  auto id = _spatial_map.at(PeppKey(oldKey));
  //  Nothing located at old location, just return
  if (!id.has_value()) return;

  //  Erase old data
  _spatial_map.remove(id.value());

  auto *cell = getDiagramProps(oldKey);
  if (cell == nullptr) return;
  _cells.erase(id.value());

  //  Move cell into new location
  PeppRect key{newKey};
  id = _spatial_map.try_add(key);

  _cells.insert({id.value(), cell});
}
