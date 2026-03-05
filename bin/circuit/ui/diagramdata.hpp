#pragma once

#include <QMap>

#include "diagramkey.hpp"
#include "diagramproperty.hpp"

class DiagramData : QObject
{
    Q_OBJECT

    //  Container for iteration
    QList<DiagramProperties *> _data;

    //  Map to data based on table location (DiagramKey)
    QMap<DiagramKey, DiagramProperties *> _cells;

    //  Map unique diagram id to table key (location)
    QMap<quint32, DiagramKey> _keys;


public:
    DiagramData();

    QList<DiagramProperties *> &cells() { return _data; }
    const QList<DiagramProperties *> &cells() const { return _data; }

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
