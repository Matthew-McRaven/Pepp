#pragma once

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h> // Required header for QML_ELEMENT

#include "schematic/component.hpp"

class CircuitProject;

class ComponentPropertyModel : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(CircuitProject *project READ project WRITE setProject NOTIFY projectChanged);
  // Q_PROPERTY(Component *component READ component WRITE setComponent NOTIFY componentChanged);
  Q_PROPERTY(u32 componentId READ componentId WRITE setComponentId NOTIFY componentChanged);

  CircuitProject *_project = nullptr;
  // Component *_component = nullptr;
  schematic::ComponentID _componentId{};

public:
  enum Role { Name = Qt::DisplayRole, Id = Qt::UserRole + 1, Direction };
  explicit ComponentPropertyModel(QObject *parent = nullptr);

  // Basic functionality:
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;

  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  QHash<int, QByteArray> roleNames() const override;

  CircuitProject *project() const { return _project; }
  void setProject(CircuitProject *project = nullptr);

  // Component *component() const { return _component; }
  // void setComponent(Component *_component = nullptr);
  u32 componentId() const { return _componentId.value; }
  void setComponentId(u32 componentId);

signals:
  void projectChanged();
  void componentChanged();
};
