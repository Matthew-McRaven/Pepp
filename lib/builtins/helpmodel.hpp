#pragma once

#include <QAbstractItemModel>
#include <QtQmlIntegration>
#include <qsortfilterproxymodel.h>
#include "builtins/constants.hpp"

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
  HelpEntry(HelpCategory::Category category, int tags, QString displayName, QString delegate)
      : category(category), tags(tags), displayName(displayName), sortName(displayName), delegate(delegate) {}
  HelpCategory::Category category;
  // TBD on how to filter these items.
  uint32_t tags = -1;
  // Display name in help system; path to QML file which can display it.
  QString displayName, sortName;
  QString delegate;
  // Props which will be injected into the delegate.
  QVariantMap props = {};
  void addChild(QSharedPointer<HelpEntry> child);
  void addChildren(QVector<QSharedPointer<HelpEntry>> children);
  // TODO: remove when all are no longer WIP.
  bool isWIP = false, isHotLoaded = false;

private:
  friend HelpModel;
  QWeakPointer<HelpEntry> _parent = {};
  // Items which occur under this item.
  QList<QSharedPointer<HelpEntry>> _children = {};
};

class HelpModel : public QAbstractItemModel {
  Q_OBJECT
  QML_ELEMENT

public:
  enum class Roles { Category = Qt::UserRole + 1, Tags, Name, Delegate, Props, Sort, WIP, HotLoaded };
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
  void addToIndex(QSharedPointer<HelpEntry>);
  inline HelpEntry *ptr(const QModelIndex &index) const {
    if (_indices.contains(index.internalId())) return static_cast<HelpEntry *>(index.internalPointer());
    return nullptr;
  }
  QSet<ptrdiff_t> _indices;
};

class HelpFilterModel : public QSortFilterProxyModel {
  Q_OBJECT
  Q_PROPERTY(builtins::Architecture architecture READ architecture WRITE setArchitecture NOTIFY architectureChanged)
  Q_PROPERTY(builtins::Abstraction abstraction READ abstraction WRITE setAbstraction NOTIFY abstractionChanged)
  Q_PROPERTY(QAbstractItemModel *model READ sourceModel WRITE setSourceModel NOTIFY sourceModelChanged)
  Q_PROPERTY(bool showWIPItems READ showWIPItems WRITE setShowWIPItems NOTIFY showWIPItemsChanged)
  QML_NAMED_ELEMENT(FilteredHelpModel)

public:
  explicit HelpFilterModel(QObject *parent = nullptr);

  void setSourceModel(QAbstractItemModel *sourceModel) override;
  builtins::Architecture architecture() const;
  void setArchitecture(builtins::Architecture architecture);
  builtins::Abstraction abstraction() const;
  void setAbstraction(builtins::Abstraction abstraction);
  bool showWIPItems() const;
  void setShowWIPItems(bool show);

protected:
  bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
  bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
signals:
  void sourceModelChanged();
  void architectureChanged();
  void abstractionChanged();
  void showWIPItemsChanged();

private:
  builtins::Architecture _architecture = builtins::Architecture::NONE;
  builtins::Abstraction _abstraction = builtins::Abstraction::NONE;
  bool _showWIPItems = false;
};
