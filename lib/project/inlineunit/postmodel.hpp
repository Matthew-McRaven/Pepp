#pragma once
#include <QAbstractListModel>
#include <QHash>
#include <QSortFilterProxyModel>
#include <QtQmlIntegration>
#include "toolchain/symbol/types.hpp"

namespace ELFIO {
class elfio;
}
class QItemSelectionModel;
class PostModel : public QAbstractTableModel {
  Q_OBJECT

  Q_PROPERTY(qsizetype longest READ longest NOTIFY longestChanged)

  struct Entry {
    QString test;
    bool cachedValue;
  };
  QList<Entry> _entries;
  qsizetype _longest{0};

public:
  // Define the role names to be used
  enum RoleNames : quint32 {
    TestRole = Qt::UserRole,
    ValueRole,
    IndexRole,
    ValidRole,
  };
  Q_ENUM(RoleNames)

  explicit PostModel(QObject *parent = nullptr);
  void clearData();
  // QAbstractItemModel interface
  int rowCount(const QModelIndex &parent) const override;
  int columnCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  qsizetype longest() const;

signals:
  void longestChanged();

protected:
  QHash<int, QByteArray> roleNames() const override;
};

class PostReshapeModel : public QAbstractProxyModel {
  Q_OBJECT
  Q_PROPERTY(PostModel *sourceModel READ castedSourceModel WRITE setSourceModel NOTIFY sourceModelChanged)
  Q_PROPERTY(qsizetype longest READ longest NOTIFY longestChanged)
  QML_NAMED_ELEMENT(PostReshapeModel)

public:
  explicit PostReshapeModel(QObject *parent = nullptr);
  ~PostReshapeModel() override = default;
  PostModel *castedSourceModel();
  void setSourceModel(QAbstractItemModel *sourceModel) override;

  qsizetype longest() const;

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
  void copyColumnCountChanged();

private:
  int _columnCount{1};
};
