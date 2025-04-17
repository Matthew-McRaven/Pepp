#pragma once
#include <QAbstractListModel>
#include <QHash>
#include <QSortFilterProxyModel>
#include <QtQmlIntegration>
#include "asm/symbol/types.hpp"

namespace ELFIO {
class elfio;
}
class QItemSelectionModel;
class StaticSymbolModel : public QAbstractTableModel {
  Q_OBJECT

  Q_PROPERTY(qsizetype longest READ longest NOTIFY longestChanged)

  struct Entry {
    // Should the symbol be visible outside its declared scope?
    symbol::Binding binding = symbol::Binding::kLocal;
    uint64_t value;
    QString name;
    QString scope; // Which symbol table did it come from
  };
  QList<Entry> _entries;
  qsizetype _longest{0};

public:
  // Define the role names to be used
  enum RoleNames : quint32 {
    SymbolRole = Qt::UserRole,
    ValueRole,
    SymbolBindingRole,
    ScopeRole,
    IndexRole,
  };
  Q_ENUM(RoleNames)

  StaticSymbolModel(QObject *parent = nullptr);
  void setFromElf(ELFIO::elfio *elf);
  void clearData();
  // QAbstractItemModel interface
  int rowCount(const QModelIndex &parent) const override;
  int columnCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  qsizetype longest() const;

signals:
  void longestChanged();

protected:
  QHash<int, QByteArray> roleNames() const override;
};

class StaticSymbolFilterModel : public QSortFilterProxyModel {
  Q_OBJECT
  Q_PROPERTY(StaticSymbolModel *sourceModel READ castedSourceModel WRITE setSourceModel NOTIFY sourceModelChanged)
  Q_PROPERTY(QString scopeFilter READ scopeFilter WRITE setScopeFilter NOTIFY scopeFilterChanged)
  Q_PROPERTY(qsizetype longest READ longest NOTIFY longestChanged)
  QML_NAMED_ELEMENT(StaticSymbolFilterModel);

public:
  explicit StaticSymbolFilterModel(QObject *parent = nullptr);
  StaticSymbolModel *castedSourceModel();
  void setSourceModel(QAbstractItemModel *sourceModel) override;
  QString scopeFilter() const;
  void setScopeFilter(const QString &scopeFilter);
  qsizetype longest() const;

protected:
  bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
signals:
  void longestChanged();
  void scopeFilterChanged();
  void sourceModelChanged();

private:
  QString _scopeFilter;
};

class StaticSymbolReshapeModel : public QAbstractProxyModel {
  Q_OBJECT
  Q_PROPERTY(StaticSymbolFilterModel *sourceModel READ castedSourceModel WRITE setSourceModel NOTIFY sourceModelChanged)
  Q_PROPERTY(qsizetype longest READ longest NOTIFY longestChanged)
  QML_NAMED_ELEMENT(StaticSymbolReshapeModel);

public:
  explicit StaticSymbolReshapeModel(QObject *parent = nullptr);
  ~StaticSymbolReshapeModel() override = default;

  StaticSymbolFilterModel *castedSourceModel();
  void setSourceModel(QAbstractItemModel *sourceModel) override;

  qsizetype longest() const;
  // Helper method that is only here because I don't want another global helper class
  Q_INVOKABLE void selectRectangle(QItemSelectionModel *selectionModel, const QModelIndex &topLeft,
                                   const QModelIndex &bottomRight) const;
  Q_INVOKABLE void copy(const QList<QModelIndex> &indices) const;

  int rowCount(const QModelIndex &parent) const override;
  int columnCount(const QModelIndex &parent) const override;
  Q_INVOKABLE void setColumnCount(int count);
  QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;
  QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override;
  QModelIndex index(int row, int column, const QModelIndex &parent) const override;
  QModelIndex parent(const QModelIndex &child) const override;
  QVariant data(const QModelIndex &index, int role) const override;

signals:
  void longestChanged();
  void sourceModelChanged();

private:
  int _columnCount{1};
};
