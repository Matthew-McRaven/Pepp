#include "helpmodel.hpp"
#include "helpdata.hpp"

static const bool dbg = false;
void HelpEntry::addChild(QSharedPointer<HelpEntry> child) {
  _children.push_back(child);
  child->_parent = this->sharedFromThis();
}

void HelpEntry::addChildren(QList<QSharedPointer<HelpEntry>> children) {
  for (auto &child : children) addChild(child);
}

HelpModel::HelpModel(QObject *parent) : QAbstractItemModel{parent} {
  _roots = {
      about_root(), writing_root(), debugging_root(), systemcalls_root(), greencard10_root(), examples_root(),
  };
}

QModelIndex HelpModel::index(int row, int column, const QModelIndex &parent) const {
  QModelIndex ret;
  if (row < 0 || column != 0) {
  } else if (!parent.isValid() && row < _roots.size()) ret = createIndex(row, column, _roots[row].data());
  else if (auto casted = (HelpEntry *)parent.internalPointer(); row < casted->_children.size())
    ret = createIndex(row, column, casted->_children[row].data());
  if constexpr (dbg) qDebug() << "HelpModel::index(" << row << "," << column << "," << parent << " ) ->" << ret;
  return ret;
}

QModelIndex HelpModel::parent(const QModelIndex &child) const {
  QModelIndex ret{};
  if (!child.isValid()) {
  } else if (auto cast_child = static_cast<HelpEntry *>(child.internalPointer()); false) {
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
  else ret = static_cast<HelpEntry *>(parent.internalPointer())->_children.size();
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
  auto entry = static_cast<HelpEntry *>(index.internalPointer());
  switch (role) {
  case (int)Roles::Category: return static_cast<int>(entry->category);
  case (int)Roles::Tags: return entry->tags;
  case Qt::DisplayRole: [[fallthrough]];
  case (int)Roles::Name: return entry->name;
  case (int)Roles::Delegate: return entry->delgate;
  case (int)Roles::Props: return entry->props;
  }
  return QVariant();
}

QHash<int, QByteArray> HelpModel::roleNames() const {
  auto ret = QAbstractItemModel::roleNames();
  ret[static_cast<int>(Roles::Category)] = "category";
  ret[static_cast<int>(Roles::Tags)] = "tags";
  ret[static_cast<int>(Roles::Name)] = "name";
  ret[static_cast<int>(Roles::Delegate)] = "delegate";
  ret[static_cast<int>(Roles::Props)] = "props";
  return ret;
}
