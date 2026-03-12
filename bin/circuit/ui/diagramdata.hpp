#pragma once

#include <list>
#include <map>

#include "core/math/geom/rectangle.hpp"
#include "core/math/geom/spatial_map.hpp"
#include "diagramproperty.hpp"

using PeppKey = pepp::core::Rectangle<i16>; //  Spatial key based on rectangle
using PeppPt = pepp::core::Point<i16>;
using PeppSize = pepp::core::Size<i16>;
using PeppId = u32; //  Unique id used to lookup cell

class DiagramData {
  //  Container for iteration
  std::list<std::unique_ptr<BaseProperties>> _data;

  //  Map to data based on table location (DiagramKey)
  std::map<PeppId, BaseProperties *> _cells;

  //  Map unique diagram id to table key (location)
  pepp::core::SpatialMap _spatial_map;

public:
  DiagramData();

  auto &cells() { return _data; }
  const auto &cells() const { return _data; }

  //  Get access to specific property
  BaseProperties *getProps(const PeppKey &key);
  const BaseProperties *getProps(const PeppKey &key) const;
  DiagramProperties *createDiagramProps(const PeppKey &key);
  LineProperties *createLineProps(const PeppKey &key);

  //  Size of canvas in logic units
  auto boundingRect() const { return _spatial_map.bounding_box(); }

  bool empty() const;
  bool clearData(const PeppKey &key);
  void moveData(const PeppKey &oldKey, const PeppKey &newKey);
};
