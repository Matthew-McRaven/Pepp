#pragma once

#include <QAbstractItemModel>
class HelpModel;
class HelpCategory : public QObject {
  Q_OBJECT
public:
  enum class Category {
    Root,
    About,
    Figure,
    ISAGreenCard,
    Text,
  };
  Q_ENUM(Category)
};

class HelpEntry : public QEnableSharedFromThis<HelpEntry> {
public:
  HelpEntry(HelpCategory::Category category, int tags, QString name, QString delgate)
      : category(category), tags(tags), name(name), delgate(delgate) {}
  HelpCategory::Category category;
  // TBD on how to filter these items.
  int tags;
  // Display name in help system; path to QML file which can display it.
  QString name, delgate;
  // Props which will be injected into the delegate.
  QVariantMap props = {};
  void addChild(QSharedPointer<HelpEntry> child);
  void addChildren(QVector<QSharedPointer<HelpEntry>> children);

private:
  friend HelpModel;
  QWeakPointer<HelpEntry> _parent = {};
  // Items which occur under this item.
  QList<QSharedPointer<HelpEntry>> _children = {};
};

class HelpModel : public QAbstractItemModel {
  Q_OBJECT
public:
  enum class Roles { Category = Qt::UserRole + 1, Tags, Name, Delegate, Props };
  Q_ENUM(Roles);
  explicit HelpModel(QObject *parent = nullptr);
  QModelIndex index(int row, int column, const QModelIndex &parent) const override;
  QModelIndex parent(const QModelIndex &child) const override;
  int rowCount(const QModelIndex &parent) const override;
  int columnCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

private:
  QList<QSharedPointer<HelpEntry>> _roots;
};
