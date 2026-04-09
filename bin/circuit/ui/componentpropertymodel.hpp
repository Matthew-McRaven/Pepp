#pragma once

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h> // Required header for QML_ELEMENT

#include "schematic/circuitproject.hpp"

class ComponentPropertyModel : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(CircuitProject *project READ project WRITE setProject NOTIFY projectChanged);

  CircuitProject *_project = nullptr;

public:
  enum Role { Name = Qt::DisplayRole, Id = Qt::UserRole + 1, Direction };
  explicit ComponentPropertyModel(QObject *parent = nullptr);

  // Basic functionality:
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;

  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

  // Editable:
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

  Qt::ItemFlags flags(const QModelIndex &index) const override;

  CircuitProject *project() const { return _project; }
  void setProject(CircuitProject *project = nullptr);

signals:
  void projectChanged();
};
