#include "helpmodel.hpp"
#include "./registry.hpp"
#include "helpdata.hpp"
#include "settings/settings.hpp"
#include "toolchain/helpers/assemblerregistry.hpp"

static const bool dbg = false;

void HelpEntry::addChild(QSharedPointer<HelpEntry> child) {
  _children.push_back(child);
  child->_parent = this->sharedFromThis();
}

void HelpEntry::addChildren(QList<QSharedPointer<HelpEntry>> children) {
  for (auto &child : children) addChild(child);
}

HelpModel::HelpModel(QObject *parent) : QAbstractItemModel{parent} {
  auto set = pepp::settings::AppSettings();
  auto figDirectory = set.general()->figureDirectory();
  _reg = helpers::registry_with_assemblers(true, figDirectory);

  // If you update the following array, YOU MUST UPDATE THE INDEX OF VARIABLE TOO!!!
  _roots = {
      navigating_root(), editing_root(), debugging_root(), greencard10_root(), examples_root(*_reg), macros_root(*_reg),
  };
  _indexOfFigs = 4;
  _indexOfMacros = 5;

  for (auto &root : _roots) addToIndex(root);
  QObject::connect(set.general(), &pepp::settings::GeneralCategory::showDebugComponentsChanged, this,
                   &HelpModel::onReloadFigures);
  QObject::connect(set.general(), &pepp::settings::GeneralCategory::allowExternalFiguresChanged, this,
                   &HelpModel::onReloadFigures);
  QObject::connect(set.general(), &pepp::settings::GeneralCategory::externalFigureDirectoryChanged, this,
                   &HelpModel::onReloadFigures);
}

QModelIndex HelpModel::index(int row, int column, const QModelIndex &parent) const {
  QModelIndex ret;
  if (row < 0 || column != 0) {
  } else if (!parent.isValid() && row < _roots.size()) ret = createIndex(row, column, _roots[row].data());
  else if (auto casted = ptr(parent); row < casted->_children.size())
    ret = createIndex(row, column, casted->_children[row].data());
  if constexpr (dbg) qDebug() << "HelpModel::index(" << row << "," << column << "," << parent << " ) ->" << ret;
  return ret;
}

QModelIndex HelpModel::parent(const QModelIndex &child) const {
  QModelIndex ret{};
  if (!child.isValid()) {
  } else if (auto cast_child = ptr(child); false) {
  } else if (auto parent = cast_child->_parent.lock(); !parent) {
  } else if (auto grandparent = parent->_parent.lock(); grandparent)
    ret = createIndex(grandparent->_children.indexOf(parent), 0, parent.data());
  else ret = createIndex(_roots.indexOf(parent), 0, parent.data());
  if constexpr (dbg) qDebug() << u"HelpModel::parent(" << child << " ) ->" << ret;
  return ret;
}

int HelpModel::rowCount(const QModelIndex &parent) const {
  auto ret = 0;
  if (!parent.isValid()) ret = _roots.size();
  else ret = ptr(parent)->_children.size();
  if constexpr (dbg) qDebug() << "HelpModel::rowCount(" << parent << " ) ->" << ret;
  return ret;
}

int HelpModel::columnCount(const QModelIndex &parent) const {
  auto ret = 1;
  if constexpr (dbg) qDebug() << "HelpModel::columnCount(" << parent << " ) ->" << ret;
  return ret;
}

QVariant HelpModel::data(const QModelIndex &index, int role) const {
  if constexpr (dbg) qDebug() << "HelpModel::data" << index << role;
  if (!index.isValid()) return QVariant();
  auto entry = ptr(index);
  // After we do a reload, QML will still temporarily hold a pointer to the old entry.
  // Must guard against accessing free'd memory.
  if (entry == nullptr) return QVariant();
  switch (role) {
  case (int)Roles::Category: return static_cast<int>(entry->category);
  case (int)Roles::Tags: return entry->tags;
  case Qt::DisplayRole: [[fallthrough]];
  case (int)Roles::Name:
    if (entry->isExternal) return entry->displayName + " (External)";
    return entry->displayName;
  case (int)Roles::Sort: return entry->sortName;
  case (int)Roles::Delegate: return entry->delegate;
  case (int)Roles::Props: return entry->props;
  case (int)Roles::WIP: return entry->isWIP;
  case (int)Roles::External: return entry->isExternal; ;
  }
  return QVariant();
}

QHash<int, QByteArray> HelpModel::roleNames() const {
  auto ret = QAbstractItemModel::roleNames();
  ret[static_cast<int>(Roles::Category)] = "category";
  ret[static_cast<int>(Roles::Tags)] = "tags";
  ret[static_cast<int>(Roles::Name)] = "name";
  ret[static_cast<int>(Roles::Sort)] = "sortName";
  ret[static_cast<int>(Roles::Delegate)] = "delegate";
  ret[static_cast<int>(Roles::Props)] = "props";
  ret[static_cast<int>(Roles::WIP)] = "isWIP";
  ret[static_cast<int>(Roles::External)] = "isExternal";
  return ret;
}

void HelpModel::onReloadFigures() {
  beginResetModel();
  auto &figs = _roots[_indexOfFigs];
  auto &macros = _roots[_indexOfMacros];
  // Must remove from index otherwise we might deref free'd in data()
  removeFromIndex(figs);
  removeFromIndex(macros);

  // Cleanup old entries before their associated registry is destroyed to prevent dangling pointers.
  figs.clear();
  macros.clear();

  // Construct registry with new settings
  _reg = helpers::registry_with_assemblers(true, pepp::settings::AppSettings().general()->figureDirectory());
  // Re-construct figures and macros in-place, inserting them into our pointer index.
  addToIndex(figs = examples_root(*_reg));
  addToIndex(macros = macros_root(*_reg));
  endResetModel();
}

void HelpModel::addToIndex(QSharedPointer<HelpEntry> entry) {
  _indices.insert(reinterpret_cast<ptrdiff_t>(entry.data()));
  for (auto &child : entry->_children) addToIndex(child);
}

void HelpModel::removeFromIndex(QSharedPointer<HelpEntry> entry) {
  _indices.remove(reinterpret_cast<ptrdiff_t>(entry.data()));
  for (auto &child : entry->_children) removeFromIndex(child);
}

HelpFilterModel::HelpFilterModel(QObject *parent) : QSortFilterProxyModel(parent) {}

void HelpFilterModel::setSourceModel(QAbstractItemModel *sourceModel) {
  if (sourceModel == this->sourceModel()) return;
  QSortFilterProxyModel::setSourceModel(sourceModel);
  QSortFilterProxyModel::setSortRole((int)HelpModel::Roles::Sort);
  emit sourceModelChanged();
}

pepp::Architecture HelpFilterModel::architecture() const { return _architecture; }

void HelpFilterModel::setArchitecture(pepp::Architecture architecture) {
  if (_architecture == architecture) return;
  _architecture = architecture;
  invalidateRowsFilter();
  emit architectureChanged();
}

pepp::Abstraction HelpFilterModel::abstraction() const { return _abstraction; }

void HelpFilterModel::setAbstraction(pepp::Abstraction abstraction) {
  if (_abstraction == abstraction) return;
  _abstraction = abstraction;
  invalidateRowsFilter();
  // qDebug() << "Mask is " << QString::number(bitmask(_architecture, _abstraction), 16).toStdString().c_str();
  emit abstractionChanged();
}

bool HelpFilterModel::showWIPItems() const { return _showWIPItems; }

void HelpFilterModel::setShowWIPItems(bool show) {
  if (_showWIPItems == show) return;
  _showWIPItems = show;
  invalidateRowsFilter();
  emit showWIPItemsChanged();
}

bool HelpFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
  auto sm = sourceModel();
  if (!sm) return false;
  int32_t mask = bitmask(_architecture, _abstraction);
  auto index = sm->index(source_row, 0, source_parent);
  auto tags = sm->data(index, (int)HelpModel::Roles::Tags).toUInt();
  if (!_showWIPItems && sm->data(index, (int)HelpModel::Roles::WIP).toBool()) return false;
  else return masked(mask, tags);
}

bool HelpFilterModel::lessThan(const QModelIndex &left, const QModelIndex &right) const {
  if (!left.parent().isValid() || !right.parent().isValid()) return left.row() < right.row();
  else return QSortFilterProxyModel::lessThan(left, right);
}
