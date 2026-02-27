#include "postmodel.hpp"
#include <QClipboard>
#include <QGuiApplication>
#include <QItemSelection>
#include <QItemSelectionModel>

PostModel::PostModel(QObject *parent) : QAbstractTableModel(parent) {}

void PostModel::clearData() {
  beginResetModel();
  _entries.clear();
  _longest = 0;
  endResetModel();
}

int PostModel::rowCount(const QModelIndex &) const { return _entries.size(); }

int PostModel::columnCount(const QModelIndex &) const { return 1; }

QVariant PostModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) return QVariant();
  auto offset = index.row();

  if (offset >= _entries.size()) {
    if (role == IndexRole) return -1;
    else return "";
  }

  auto entry = _entries.at(offset);
  switch (role) {
  case TestRole: return entry.test;
  case ValueRole: return entry.cachedValue;
  case IndexRole: return index.row();
  case ValidRole: return true;
  default: break;
  }
  return {};
}

qsizetype PostModel::longest() const { return _longest; }

void PostModel::updateValuesFromVector(const std::vector<bool> &values) {
  auto rowCount = std::min<int>(values.size(), _entries.size());
  for (int i = 0; i < rowCount; i++) _entries[i].cachedValue = values[i];
  auto tl = index(0, 0), br = index(rowCount - 1, 0);
  emit dataChanged(tl, br);
}

void PostModel::resetFromVector(QStringList names) {
  beginResetModel();
  _entries.resize(names.size());
  _longest = 0;
  for (int it = 0; it < names.size(); it++) {
    _entries[it].test = names[it];
    _entries[it].cachedValue = -1;
    _longest = std::max(_longest, static_cast<qsizetype>(names[it].size()));
  }
  emit longestChanged();
  endResetModel();
}

QHash<int, QByteArray> PostModel::roleNames() const {
  static const auto roles = QHash<int, QByteArray>{
      {(int)TestRole, "test"}, {(int)ValueRole, "value"}, {(int)IndexRole, "index"}, {(int)ValidRole, "valid"}};
  return roles;
}

PostReshapeModel::PostReshapeModel(QObject *parent) : QAbstractProxyModel(parent) {}

PostModel *PostReshapeModel::castedSourceModel() { return dynamic_cast<PostModel *>(sourceModel()); }

void PostReshapeModel::setSourceModel(QAbstractItemModel *sourceModel) {
  if (sourceModel == this->sourceModel()) return;
  auto old = castedSourceModel();
  if (old) disconnect(old, nullptr, this, nullptr);

  QAbstractProxyModel::setSourceModel(sourceModel);
  if (auto casted = dynamic_cast<PostModel *>(sourceModel); casted != nullptr) {
    auto reset_model = [this]() {
      beginResetModel();
      endResetModel();
    };
    auto data_changed = [this](const QModelIndex &tl, const QModelIndex &br) {
      // Selection is a rect.
      // Rather than emit multiple dataChanged (1 for partial first row, 1 for all complete rows, 1 for partial last
      // row), just round up to the rect covering all items in the affected rows.
      auto mapped_tl = mapFromSource(tl).siblingAtColumn(0),
           mapped_br = mapFromSource(br).siblingAtColumn(_columnCount - 1);
      if (mapped_tl.isValid() && mapped_br.isValid()) emit dataChanged(mapped_tl, mapped_br);
    };
    connect(casted, &PostModel::longestChanged, this, &PostReshapeModel::longestChanged);
    connect(casted, &PostModel::modelReset, this, reset_model);
    connect(casted, &PostModel::dataChanged, this, data_changed);
    emit sourceModelChanged();
  }
}

qsizetype PostReshapeModel::longest() const {
  if (auto model = dynamic_cast<PostModel *>(sourceModel()); model == nullptr) return 0;
  else return model->longest();
}

int PostReshapeModel::rowCount(const QModelIndex &parent) const {
  if (auto model = dynamic_cast<PostModel *>(sourceModel()); !model) return 0;
  else return (model->rowCount(mapToSource(parent)) + (_columnCount - 1)) / _columnCount;
}

int PostReshapeModel::columnCount(const QModelIndex &) const { return _columnCount; }

void PostReshapeModel::setColumnCount(int count) {
  if (count == _columnCount || count <= 0) return;
  beginResetModel();
  _columnCount = count;
  endResetModel();
  emit layoutChanged();
}

QModelIndex PostReshapeModel::mapToSource(const QModelIndex &proxyIndex) const {
  if (!proxyIndex.isValid()) return {};

  int row = proxyIndex.row(), column = proxyIndex.column();
  int sourceRow = row * _columnCount + column;

  if (!sourceModel() || sourceRow >= sourceModel()->rowCount()) return {};
  return sourceModel()->index(sourceRow, 0, {});
}

QModelIndex PostReshapeModel::mapFromSource(const QModelIndex &sourceIndex) const {
  if (!sourceIndex.isValid() || sourceIndex.column() != 0) return {};

  int sourceRow = sourceIndex.row();
  int row = sourceRow / _columnCount, column = sourceRow % _columnCount;
  return index(row, column, {});
}

QModelIndex PostReshapeModel::index(int row, int column, const QModelIndex &) const { return createIndex(row, column); }

// Flat 2D model, so no parent-children relationships.
QModelIndex PostReshapeModel::parent(const QModelIndex &) const { return {}; }

QVariant PostReshapeModel::data(const QModelIndex &index, int role) const {
  if (auto source = sourceModel(); source == nullptr) return {};
  else if (!index.isValid()) return {};
  // Convert 2D-index back to 1D before deferring to old model
  else if (auto mapped = mapToSource(index); !mapped.isValid()) {
    if (role == (int)PostModel::RoleNames::ValidRole) return false;
    return {};
  } else return source->data(mapped, role);
}
