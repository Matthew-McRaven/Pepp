/*
 * Copyright (c) 2024-2026 J. Stanley Warford, Matthew McRaven
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

#include <QAbstractItemModel>
#include <QtQmlIntegration>
#include <qsortfilterproxymodel.h>
#include "enums/constants.hpp"

namespace builtins {
class Registry;
}
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
  // Used to lookup this help entry in the model without knowing the full name.
  QString slug;
  // Props which will be injected into the delegate.
  QVariantMap props = {};
  void addChild(QSharedPointer<HelpEntry> child);
  void addChildren(QVector<QSharedPointer<HelpEntry>> children);
  // TODO: remove when all are no longer WIP.
  bool isWIP = false, isExternal = false;

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
  enum class Roles { Category = Qt::UserRole + 1, Tags, Name, Delegate, Props, Sort, WIP, External };
  Q_ENUM(Roles);
  explicit HelpModel(QObject *parent = nullptr);
  QModelIndex index(int row, int column, const QModelIndex &parent) const override;
  QModelIndex parent(const QModelIndex &child) const override;
  int rowCount(const QModelIndex &parent) const override;
  int columnCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;
  QModelIndex indexFromSlug(const QString &slug);

private:
  QList<QSharedPointer<HelpEntry>> _roots;
  int _indexOfFigs = -1, _indexOfMacros = -1;
  void addToIndex(QSharedPointer<HelpEntry>);
  void removeFromIndex(QSharedPointer<HelpEntry>);
  inline HelpEntry *ptr(const QModelIndex &index) const {
    if (_indices.contains(index.internalId())) return static_cast<HelpEntry *>(index.internalPointer());
    return nullptr;
  }
  // Registry data must outlive referrents or we get untraceable sgefaults inside QML.
  QSharedPointer<builtins::Registry> _reg = nullptr;
  QSet<ptrdiff_t> _indices;
};

class HelpFilterModel : public QSortFilterProxyModel {
  Q_OBJECT
  Q_PROPERTY(pepp::Architecture architecture READ architecture WRITE setArchitecture NOTIFY architectureChanged)
  Q_PROPERTY(pepp::Abstraction abstraction READ abstraction WRITE setAbstraction NOTIFY abstractionChanged)
  Q_PROPERTY(QAbstractItemModel *model READ sourceModel WRITE setSourceModel NOTIFY sourceModelChanged)
  Q_PROPERTY(bool showWIPItems READ showWIPItems WRITE setShowWIPItems NOTIFY showWIPItemsChanged)
  QML_NAMED_ELEMENT(FilteredHelpModel)

public:
  explicit HelpFilterModel(QObject *parent = nullptr);

  void setSourceModel(QAbstractItemModel *sourceModel) override;
  pepp::Architecture architecture() const;
  void setArchitecture(pepp::Architecture architecture);
  pepp::Abstraction abstraction() const;
  void setAbstraction(pepp::Abstraction abstraction);
  bool showWIPItems() const;
  void setShowWIPItems(bool show);
  Q_INVOKABLE QModelIndex indexFromSlug(const QString &slug);

protected:
  bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
  bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
signals:
  void sourceModelChanged();
  void architectureChanged();
  void abstractionChanged();
  void showWIPItemsChanged();

private:
  pepp::Architecture _architecture = pepp::Architecture::NO_ARCH;
  pepp::Abstraction _abstraction = pepp::Abstraction::NO_ABS;
  bool _showWIPItems = false;
};
