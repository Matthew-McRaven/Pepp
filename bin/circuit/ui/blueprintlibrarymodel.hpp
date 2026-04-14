#pragma once

#include <QAbstractListModel>
#include <Qt>
#include <QtQml/qqmlregistration.h> // Required header for QML_ELEMENT
#include <QtQmlIntegration>

#include "schematic/blueprintlibrary.hpp"
#include "schematic/circuitproject.hpp"

class BlueprintLibraryModel : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(CircuitProject *project READ project WRITE setProject NOTIFY projectChanged);
  Q_PROPERTY(u32 blueprint READ blueprint WRITE setBlueprint NOTIFY blueprintChanged);
  Q_PROPERTY(QVariant blueprintTypes READ blueprintTypes NOTIFY blueprintChanged);

public:
  enum Role { Name = Qt::DisplayRole, Path = Qt::UserRole + 1, Id };
  enum Filter { Arrow = Qt::UserRole + 1, Diagram, Line, None = 0xffffffff };
  Q_ENUM(Filter)
  explicit BlueprintLibraryModel(QObject *parent = nullptr);

  // Basic functionality:
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;

  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;
  CircuitProject *project() const { return _project; }
  void setProject(CircuitProject *project = nullptr);

  u32 blueprint() const { return _blueprintGroupId.value; }
  void setBlueprint(u32 blueprint);

  QVariant blueprintTypes() { return QVariant::fromValue(_currentBlueprints); }
  // Q_INVOKABLE QVariant blueprintCount() { return QVariant::fromValue(_currentBlueprints.size()); }
  //  Q_INVOKABLE u32 blueprint(int index) const;

signals:
  void projectChanged();
  void blueprintChanged();

private:
  void createBlueprintList();

  CircuitProject *_project = nullptr;
  schematic::BlueprintGroupID _blueprintGroupId{};
  QVariantList _currentBlueprints;
};
