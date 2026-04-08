#pragma once

#include <QAbstractListModel>
#include <Qt>
#include <QtQml/qqmlregistration.h> // Required header for QML_ELEMENT
#include <QtQmlIntegration>

#include "diagramtype.hpp"
#include "schematic/blueprint.hpp"
#include "schematic/circuitproject.hpp"

class BlueprintLibraryModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(CircuitProject *project READ project WRITE setProject NOTIFY projectChanged);
  QML_ELEMENT

public:
  enum Role { Name = Qt::DisplayRole, Path = Qt::UserRole + 1 };
  explicit BlueprintLibraryModel(QObject *parent = nullptr);

  // Basic functionality:
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;

  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;
  CircuitProject *project() const { return _project; }
  void setProject(CircuitProject *project = nullptr);

  Q_INVOKABLE u32 blueprint(int index) const;

signals:
  void projectChanged();

private:
  CircuitProject *_project = nullptr;
};

class FilterDiagramListModel : public QSortFilterProxyModel {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(Filter filter READ filterGroupType WRITE setFilterGroupFilter NOTIFY filterChanged);
  Q_PROPERTY(BlueprintLibraryModel *model READ model WRITE setModel NOTIFY modelChanged);

public:
  enum Filter { Arrow = Qt::UserRole + 1, Diagram, Line, None = 0xffffffff };
  Q_ENUM(Filter)

  explicit FilterDiagramListModel(QObject *parent = nullptr);

  // Basic functionality:
  Filter filterGroupType() const { return _filter; }
  void setFilterGroupFilter(Filter filter = Filter::None);
  BlueprintLibraryModel *model() const { return static_cast<BlueprintLibraryModel *>(sourceModel()); }
  void setModel(BlueprintLibraryModel *model = nullptr);

protected:
  bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
  // bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

signals:
  void filterChanged();
  void modelChanged();

private:
  Filter _filter = Filter::None;
  QString _filterString;
};
