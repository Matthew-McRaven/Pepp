#pragma once

#include <list>
#include <map>

#include <QMap>

#include "core/math/geom/spatial_map.hpp"
#include "diagramkey.hpp"
#include "diagramproperty.hpp"

class DiagramData {
  //  Container for iteration
  std::list<DiagramProperties> _data;

  //  Map to data based on table location (DiagramKey)
  QMap<DiagramKey, DiagramProperties *> _cells;
  pepp::core::SpatialMap _spatial_map;

  //  Map unique diagram id to table key (location)
  std::map<quint32, DiagramKey> _keys;

public:
    DiagramData();

    std::list<DiagramProperties> &cells() { return _data; }
    const std::list<DiagramProperties> &cells() const { return _data; }

    //  Get access to specific property
    DiagramProperties *getDiagramProps(const DiagramKey &key);
    DiagramProperties *createDiagramProps(const DiagramKey &key);

    bool empty() const;
    QVariant getData(int id, int role) const;
    QVariant getData(const DiagramKey &key, int role) const;
    bool setData(const DiagramKey &key, const QVariant &value, int role);
    bool clearData(const DiagramKey &key);
    void moveData(const DiagramKey &oldKey, const DiagramKey &newKey);

    /******************************************************
     * If the key already exists in the model, returns the
     * id; otherwise, adds the key, assignes an id, and
     * returns the id.
     ******************************************************/
    int createId(const DiagramKey &key);
    int getId(const DiagramKey &key) const;
    DiagramKey getKey(int id) const;
};
