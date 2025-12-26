/*
 * Copyright (c) 2025-2026 J. Stanley Warford, Matthew McRaven
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

#include <QObject>
#include <QtQmlIntegration>
#include <qqmllist.h>
#include "stack_tracer.hpp"

class ChangeTypeHelper : public QObject {
  Q_GADGET
  QML_NAMED_ELEMENT(ChangeType)
  QML_UNCREATABLE("Error:Only enums")
public:
  enum class ChangeType : uint32_t {
    None,
    Modified,
    Allocated,
  };
  Q_ENUM(ChangeType)
  ChangeTypeHelper(QObject *parent = nullptr);
};
using ChangeType = ChangeTypeHelper::ChangeType;

class ActivationModelRoles : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("")
public:
  enum RoleNames {
    // 1 if slot, 2 if frame, 3 if stack.
    NodeType = Qt::UserRole + 0,
    SlotName = Qt::UserRole + 1,
    SlotAddress = Qt::UserRole + 2,
    SlotValue = Qt::UserRole + 3,
    SlotStatus = Qt::UserRole + 4,
    FrameActive = Qt::UserRole + 5,
  };
  Q_ENUM(RoleNames)
  static ActivationModelRoles *instance();
  // Prevent copying and assignment
  ActivationModelRoles(const ActivationModelRoles &) = delete;
  ActivationModelRoles &operator=(const ActivationModelRoles &) = delete;

private:
  ActivationModelRoles() : QObject(nullptr) {}
};

class ActivationModel : public QAbstractItemModel {
  // QAbstractItemModel interface
  Q_OBJECT
  Q_PROPERTY(pepp::debug::StackTracer *stackTracer READ stackTracer WRITE setStackTracer NOTIFY stackTracerChanged)
  Q_PROPERTY(QModelIndex activeStackIndex READ activeStackIndex NOTIFY activeStackIndexChanged)
  QML_NAMED_ELEMENT(RootActivationModel)
public:
  QModelIndex index(int row, int column, const QModelIndex &parent) const override;
  QModelIndex parent(const QModelIndex &child) const override;
  int rowCount(const QModelIndex &parent) const override;
  int columnCount(const QModelIndex &parent) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  QHash<int, QByteArray> roleNames() const override;

  pepp::debug::StackTracer *stackTracer() const;
  void setStackTracer(pepp::debug::StackTracer *stackTracer);
  QModelIndex activeStackIndex() const;

public slots:
  // Call at the same time as WatchExpressionEditor::update_volatile_values
  void update_volatile_values();
signals:
  void stackTracerChanged();
  void activeStackIndexChanged();

private:
  pepp::debug::StackTracer *_stackTracer = nullptr;
  QModelIndex _activeStackIndex;
};

class ScopedActivationModel : public QAbstractProxyModel {
  Q_OBJECT
  Q_PROPERTY(QModelIndex scopeToIndex READ scopeToIndex WRITE setScopeToIndex NOTIFY scopeToIndexChanged)
  QML_NAMED_ELEMENT(ScopedActivationModel)
public:
  explicit ScopedActivationModel(QObject *parent = nullptr);
  ~ScopedActivationModel() override = default;

  void setSourceModel(QAbstractItemModel *model) override;
  QModelIndex scopeToIndex() const;
  void setScopeToIndex(const QModelIndex &index);

  QModelIndex index(int row, int column, const QModelIndex &parent) const override;
  QModelIndex parent(const QModelIndex &child) const override;
  int rowCount(const QModelIndex &parent) const override;
  int columnCount(const QModelIndex &parent) const override;
  QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;
  QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override;

signals:
  void scopeToIndexChanged();

private:
  QPersistentModelIndex _scopeToIndex;
  // Lazily construct a bidirectional mapping between source and proxy indices.
  mutable QHash<QPersistentModelIndex, QPersistentModelIndex> _source_to_proxy, _proxy_to_source;
  static bool isDescendantOf(QModelIndex idx, const QPersistentModelIndex &anc);
private slots:
  void handleSourceReset();
  /*void handleSourceLayoutChange();
  void handleSourceStructureChange();*/
  void handleSourceDataChange(const QModelIndex &tl, const QModelIndex &br, const QList<int> &roles);
};
