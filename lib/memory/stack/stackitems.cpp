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
#include "stackitems.hpp"
#include <spdlog/spdlog.h>
#include "fmt/ranges.h"

ChangeTypeHelper::ChangeTypeHelper(QObject *parent) : QObject(parent) {}

QModelIndex ActivationModel::index(int row, int column, const QModelIndex &parent) const {
  if (!_stackTracer) return QModelIndex();
  else if (row < 0 || column < 0) return QModelIndex();
  else if (column >= 1) return QModelIndex(); // We only have one column.

  if (!parent.isValid()) { // We are a stack
    if (row >= static_cast<int>(_stackTracer->size())) return QModelIndex();
    return createIndex(row, 0, _stackTracer->at(row));
  }
  auto ptr = static_cast<pepp::debug::LayoutNode *>(parent.internalPointer());
  if (auto asStack = dynamic_cast<pepp::debug::Stack *>(ptr); asStack) { // parent is stack, we are a frame
    if (row >= static_cast<int>(asStack->size())) return QModelIndex();
    return createIndex(row, 0, asStack->reverse_at(row));
  } else if (auto asFrame = dynamic_cast<pepp::debug::Frame *>(ptr); asFrame) { // parent is frame, we are a slot
    if (row >= static_cast<int>(asFrame->size())) return QModelIndex();
    return createIndex(row, 0, asFrame->reverse_at(row));
  } else return QModelIndex(); // Should not be possible
}

QModelIndex ActivationModel::parent(const QModelIndex &child) const {
  if (!child.isValid()) return QModelIndex();
  else if (!_stackTracer) return QModelIndex();

  auto ptr = static_cast<pepp::debug::LayoutNode *>(child.internalPointer());
  int stackIdx = 0;
  if (auto asStack = dynamic_cast<pepp::debug::Stack *>(ptr); asStack) { // We are a stack, parent does not exist
    return QModelIndex();
  } else if (auto asFrame = dynamic_cast<pepp::debug::Frame *>(ptr); asFrame) { // We are a frame, parent is a stack
    auto parent = asFrame->parent();

    std::optional<std::size_t> parentRow = std::nullopt;
    std::size_t stackIdx = 0;
    for (const auto &stack : *_stackTracer) {
      if (stack.get() == parent) {
        parentRow = stackIdx;
        break;
      }
      stackIdx++;
    }

    if (!parentRow) return QModelIndex();
    return createIndex(*parentRow, 0, parent);
  } else if (auto asSlot = dynamic_cast<pepp::debug::Slot *>(ptr); asSlot) { // We are a slot, parent is a frame
    auto parentFrame = asSlot->parent();
    if (!parentFrame) return QModelIndex();
    auto parentStack = parentFrame->parent();
    if (!parentStack) return QModelIndex();

    std::optional<std::size_t> parentRow = std::nullopt;
    std::size_t frameIdx = 0;
    for (const auto &frame : *parentStack) {
      if (&frame == parentFrame) {
        parentRow = frameIdx;
        break;
      }
      frameIdx++;
    }

    if (!parentRow) return QModelIndex();
    return createIndex(*parentRow, 0, parentFrame);
  } else return QModelIndex();
}

int ActivationModel::rowCount(const QModelIndex &parent) const {
  if (!parent.isValid()) return _stackTracer->size();

  auto ptr = static_cast<pepp::debug::LayoutNode *>(parent.internalPointer());
  if (auto asStack = dynamic_cast<pepp::debug::Stack *>(ptr); asStack) { // parent is stack
    return asStack->size();
  } else if (auto asFrame = dynamic_cast<pepp::debug::Frame *>(ptr); asFrame) { // parent is frame
    return asFrame->size();
  } else return 0; // parent is record?
}

int ActivationModel::columnCount(const QModelIndex &parent) const { return 1; }

QVariant ActivationModel::data(const QModelIndex &index, int role) const {
  int row = index.row(), col = index.column();
  if (!index.isValid() || row < 0 || col != 0) return QVariant();
  auto ptr = static_cast<pepp::debug::LayoutNode *>(index.internalPointer());

  pepp::debug::Slot *asSlot = dynamic_cast<pepp::debug::Slot *>(ptr);
  pepp::debug::Frame *asFrame = dynamic_cast<pepp::debug::Frame *>(ptr);
  pepp::debug::Stack *asStack = dynamic_cast<pepp::debug::Stack *>(ptr);

  using AMR = ActivationModelRoles::RoleNames;
  switch (role) {
  case (int)AMR::NodeType:
    if (asSlot) return 1;
    else if (asFrame) return 2;
    else if (asStack) return 3;
    else return QVariant();

  case (int)AMR::SlotName:
    if (asSlot) return QVariant::fromValue(asSlot->name());
    else return QVariant();

  case (int)AMR::SlotAddress:
    if (asSlot) return QVariant::fromValue(asSlot->address());
    else return QVariant();

  case (int)AMR::SlotValue:
    if (asSlot) return QVariant::fromValue(QString::fromStdString(asSlot->value()));
    else return QVariant();

  case (int)AMR::SlotStatus: {
    if (asSlot) {
      auto enumValue =
          asSlot->is_value_dirty() ? ChangeTypeHelper::ChangeType::Modified : ChangeTypeHelper::ChangeType::None;
      return QVariant::fromValue(enumValue);
    } else return QVariant();
  }
  case (int)AMR::FrameActive:
    if (asFrame) return asFrame->active();
    else return QVariant();
  }
  return QVariant();
}

QHash<int, QByteArray> ActivationModel::roleNames() const {
  using AMR = ActivationModelRoles::RoleNames;
  QHash<int, QByteArray> ret = {{(int)AMR::NodeType, "type"},       {(int)AMR::SlotName, "name"},
                                {(int)AMR::SlotAddress, "address"}, {(int)AMR::SlotValue, "value"},
                                {(int)AMR::SlotStatus, "status"},   {(int)AMR::FrameActive, "active"}};
  return ret;
}

pepp::debug::StackTracer *ActivationModel::stackTracer() const { return _stackTracer; }

void ActivationModel::setStackTracer(pepp::debug::StackTracer *stackTracer) {
  if (_stackTracer == stackTracer) return;
  _stackTracer = stackTracer;
  update_volatile_values();
  emit stackTracerChanged();
}

QModelIndex ActivationModel::activeStackIndex() const { return _activeStackIndex; }

void ActivationModel::update_volatile_values() {
  if (_stackTracer) {
    _stackTracer->update_volatile_values();
    auto activeStackIndex = _stackTracer->activeStackIndex();
    QModelIndex mi{};
    if (activeStackIndex) mi = createIndex(*activeStackIndex, 0, _stackTracer->activeStack());
    if (mi != _activeStackIndex) {
      _activeStackIndex = mi;
      emit activeStackIndexChanged();
    }
    auto activeStack = _stackTracer->activeStack();
    /*if (activeStack) {
      auto joined_lines = activeStack->to_string(0);
      auto joiner = "\n    ";
      spdlog::get("debugger::stack")->info("{}{}", joiner, fmt::join(joined_lines.begin(), joined_lines.end(), joiner));
    }*/
    emit dataChanged(index(0, 0, {}), index(rowCount({}) - 1, 0, {}));
  }
}

ScopedActivationModel::ScopedActivationModel(QObject *parent) : QAbstractProxyModel(parent) {}

void ScopedActivationModel::setSourceModel(QAbstractItemModel *model) {
  if (model == sourceModel()) return;
  // spdlog::info("ScopedActivationModel::setSourceModel({})", (void *)model);
  beginResetModel();
  if (sourceModel()) disconnect(sourceModel(), nullptr, this, nullptr);
  QAbstractProxyModel::setSourceModel(model);

  _scopeToIndex = QPersistentModelIndex(); // clear; caller should set a new root
  _source_to_proxy.clear();
  _proxy_to_source.clear();
  if (model) {
    connect(model, &QAbstractItemModel::modelReset, this, &ScopedActivationModel::handleSourceReset);
    // connect(model, &QAbstractItemModel::layoutChanged, this, &ScopedActivationModel::handleSourceLayoutChange);
    // connect(model, &QAbstractItemModel::rowsInserted, this, &ScopedActivationModel::handleSourceStructureChange);
    // connect(model, &QAbstractItemModel::rowsRemoved, this, &ScopedActivationModel::handleSourceStructureChange);
    connect(model, &QAbstractItemModel::dataChanged, this, &ScopedActivationModel::handleSourceDataChange);
  }
  endResetModel();
}

QModelIndex ScopedActivationModel::scopeToIndex() const { return _scopeToIndex; }

void ScopedActivationModel::setScopeToIndex(const QModelIndex &index) {
  if (_scopeToIndex == index) return;
  /*spdlog::info("ScopedActivationModel::setScopeToIndex({}, {}, {})", index.row(), index.column(),
               index.internalPointer());*/
  _scopeToIndex = index;
  beginResetModel();
  _source_to_proxy.clear();
  _proxy_to_source.clear();
  _source_to_proxy[_scopeToIndex] = QModelIndex();
  _proxy_to_source[QModelIndex()] = _scopeToIndex;
  endResetModel();
  emit scopeToIndexChanged();
}

// Children can never be created before a parent. This must be true, because otherwise we would not know the value for
// pi_parent. Additionally, we are gaurenteed index(...) for a item will be called before mapToSource, because we MUST
// have a proxy index to map. As we "discover" lower levels of the tree, we cache the mapping between source and proxy
// indices.
QModelIndex ScopedActivationModel::index(int row, int column, const QModelIndex &pi_parent) const {
  QModelIndex ret;
  if (!sourceModel() || row < 0 || column < 0) {
    ret = {};
  } else if (const QModelIndex si_parent = pi_parent.isValid() ? mapToSource(pi_parent) : QModelIndex(_scopeToIndex);
             !si_parent.isValid() && _scopeToIndex.isValid()) {
    // mapToSource is guarenteed to return a valid index if proxy parent is valid, because we must have created it
    // on a previous call to index(...)
    ret = {};
  } else if (QModelIndex si_idx = sourceModel()->index(row, column, si_parent); !si_idx.isValid()) {
    // Get the original source index; we want its internal pointer.
    ret = {};
  } else {
    auto pi_idx = createIndex(row, column, si_idx.internalPointer());
    if (!_proxy_to_source.contains(pi_idx)) {
      // Re-establish bi-directional mapping between proxy and source indices by adding new proxy index.
      _source_to_proxy[si_idx] = pi_idx;
      _proxy_to_source[pi_idx] = si_idx;
    }
    ret = pi_idx;
  }
  /*spdlog::info("index({}, {}, ({}, {}, {})) = ({}, {}, {})", row, column, pi_parent.isValid() ? pi_parent.row() : -1,
               pi_parent.isValid() ? pi_parent.column() : -1, pi_parent.isValid() ? pi_parent.internalPointer() : 0,
               ret.row(), ret.column(), ret.internalPointer());*/
  return ret;
}

QModelIndex ScopedActivationModel::parent(const QModelIndex &pi_child) const {
  QModelIndex ret;
  if (!sourceModel() || !pi_child.isValid()) ret = {};
  else if (QModelIndex si_child = mapToSource(pi_child), si_parent = si_child.parent(); si_parent == _scopeToIndex) {
    // If parent is the chosen root, proxy parent is invalid (top-level).
    ret = {};
  } else if (!isDescendantOf(si_parent, _scopeToIndex)) {
    // If above the root, invalid in this view.
    ret = {};
  } else ret = mapFromSource(si_parent);
  /*spdlog::info("parent({}, {}, {}) = ({}, {}, {})", pi_child.row(), pi_child.column(), pi_child.internalPointer(),
               ret.row(), ret.column(), ret.internalPointer());*/
  return ret;
}

int ScopedActivationModel::rowCount(const QModelIndex &pi_parent) const {
  if (!sourceModel()) return 0;
  auto si_parent = mapToSource(pi_parent);
  auto rc = sourceModel()->rowCount(si_parent);
  // spdlog::info("rowCount({}, {}, {}) = {}", pi_parent.row(), pi_parent.column(), pi_parent.internalPointer(), rc);
  return rc;
}

int ScopedActivationModel::columnCount(const QModelIndex &pi_parent) const {
  if (!sourceModel()) return 0;
  auto si_parent = mapToSource(pi_parent);
  return sourceModel()->columnCount(si_parent);
}

QModelIndex ScopedActivationModel::mapToSource(const QModelIndex &proxyIndex) const {
  QModelIndex ret{};
  if (!sourceModel() || !proxyIndex.isValid()) ret = _scopeToIndex;
  else if (auto si = _proxy_to_source.find(proxyIndex); si != _proxy_to_source.end()) ret = si.value();
  else ret = _scopeToIndex;
  /*spdlog::info("mapToSource({}, {}, {}) = ({}, {}, {})", proxyIndex.row(), proxyIndex.column(),
               proxyIndex.internalPointer(), ret.row(), ret.column(), ret.internalPointer());*/
  return ret;
}

QModelIndex ScopedActivationModel::mapFromSource(const QModelIndex &sourceIndex) const {
  if (!sourceModel() || !sourceIndex.isValid()) return {};
  if (!isDescendantOf(sourceIndex, _scopeToIndex)) return {};

  struct Step {
    int r, c;
  };
  // Build path from sourceIndex to _scopeToIndex
  QVector<Step> path;
  QModelIndex si_cur = sourceIndex;
  // Stop when we reach the root (si_cur is invalid), or when we reach _scopeToIndex. As an optimization, we can also
  // stop when we have a hit in _source_to_proxy, since those are all cached childen of _scopeToIndex.
  while (si_cur.isValid() && si_cur != _scopeToIndex && !_source_to_proxy.contains(si_cur)) {
    path.push_back({si_cur.row(), si_cur.column()});
    si_cur = si_cur.parent();
  }

  // if sourceIndex was not a descendent, it is not representable in this model.
  if (si_cur != _scopeToIndex) return {};

  // Walk down through proxy using index() to construct the proxy index and update caches.
  QModelIndex pi_p;
  for (int i = path.size() - 1; i >= 0; --i) pi_p = index(path[i].r, path[i].c, pi_p);
  /*spdlog::info("mapFromSource({}, {}, {}) = ({}, {}, {})", sourceIndex.row(), sourceIndex.column(),
               sourceIndex.internalPointer(), pi_p.row(), pi_p.column(), pi_p.internalPointer());*/
  return pi_p;
}

bool ScopedActivationModel::isDescendantOf(QModelIndex si_idx, const QPersistentModelIndex &si_anc) {
  if (!si_anc.isValid()) return false; // require explicit root
  for (QModelIndex si_p = si_idx; si_p.isValid(); si_p = si_p.parent())
    if (si_p == si_anc) return true;
  return false;
}

void ScopedActivationModel::handleSourceReset() {
  beginResetModel();
  // Keep _scopeToIndex if still valid; otherwise clear.
  if (_scopeToIndex.isValid() && _scopeToIndex.model() != sourceModel()) {
    _scopeToIndex = QModelIndex();
    _source_to_proxy.clear();
    _proxy_to_source.clear();
  }
  endResetModel();
}
/*

void ScopedActivationModel::handleSourceLayoutChange() { emit layoutChanged(); }

void ScopedActivationModel::handleSourceStructureChange() {
  beginResetModel();
  _source_to_proxy.clear();
  _proxy_to_source.clear();
  endResetModel();
}*/

void ScopedActivationModel::handleSourceDataChange(const QModelIndex &tl, const QModelIndex &br,
                                                   const QList<int> &roles) {
  // Emit through proxy for the visible range intersecting our subtree.
  if (!_scopeToIndex.isValid()) return;
  Q_UNUSED(tl);
  Q_UNUSED(br);
  Q_UNUSED(roles);
  beginResetModel();
  _source_to_proxy.clear();
  _proxy_to_source.clear();
  endResetModel();
}
