#include "symbolmodel.hpp"
#include <QClipboard>
#include <QGuiApplication>
#include <QItemSelection>
#include <QItemSelectionModel>
#include "elfio/elfio.hpp"

StaticSymbolModel::StaticSymbolModel(QObject *parent) : QAbstractTableModel(parent) {}

symbol::Binding bindingFromElf(unsigned char bind) {
  switch (bind) {
  case ELFIO::STB_LOCAL: return symbol::Binding::kLocal;
  case ELFIO::STB_GLOBAL: return symbol::Binding::kGlobal;
  case ELFIO::STB_WEAK: return symbol::Binding::kImported;
  default: return symbol::Binding::kLocal;
  }
}

void StaticSymbolModel::setFromElf(ELFIO::elfio *elf) {
  std::string name;
  ELFIO::Elf64_Addr value;
  ELFIO::Elf_Xword size;
  unsigned char bind, type, other;
  ELFIO::Elf_Half section_index;

  beginResetModel();
  _entries.clear();
  for (const auto &section : elf->sections) {
    if (section->get_type() != ELFIO::SHT_SYMTAB) continue;
    ELFIO::symbol_section_accessor sym_access(*elf, section.get());
    for (ELFIO::Elf_Xword index = 1; index < sym_access.get_symbols_num(); index++) {
      sym_access.get_symbol(index, name, value, size, bind, type, section_index, other);
      Entry e;
      e.name = QString::fromStdString(name);
      e.value = value;
      e.binding = bindingFromElf(bind);
      e.scope = QString::fromStdString(section->get_name());
      //  Find longest key
      _longest = std::max(_longest, e.name.length());
      _entries.append(e);
    }
  }
  //  Sort list, case insensitive
  std::sort(_entries.begin(), _entries.end(), [&](const Entry &s1, const Entry &s2) {
    return QString::compare(s1.name, s2.name, Qt::CaseInsensitive) < 0;
  });
  emit longestChanged();
  endResetModel();
}

void StaticSymbolModel::clearData() {
  beginResetModel();
  _entries.clear();
  _longest = 0;
  endResetModel();
}

int StaticSymbolModel::rowCount(const QModelIndex &parent) const { return _entries.size(); }

int StaticSymbolModel::columnCount(const QModelIndex &parent) const { return 1; }

QVariant StaticSymbolModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) return QVariant();
  auto offset = index.row();

  if (offset >= _entries.size()) {
    if (role == IndexRole) return -1;
    else return "";
  }

  auto entry = _entries.at(offset);
  switch (role) {
  case SymbolRole: return entry.name;
  case ValueRole: return QStringLiteral("%1").arg(entry.value, 4, 16, QLatin1Char('0')).toUpper();
  case IndexRole: return index.row();
  case ScopeRole: return entry.scope;
  case ValidRole: return true;
  default: break;
  }
  return {};
}

Qt::ItemFlags StaticSymbolModel::flags(const QModelIndex &index) const {
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

qsizetype StaticSymbolModel::longest() const { return _longest; }

QHash<int, QByteArray> StaticSymbolModel::roleNames() const {
  static const auto roles = QHash<int, QByteArray>{{(int)SymbolRole, "symbol"},
                                                   {(int)ScopeRole, "scope"},
                                                   {(int)ValueRole, "value"},
                                                   {(int)IndexRole, "index"},
                                                   {(int)ValidRole, "valid"}};
  return roles;
}

StaticSymbolFilterModel::StaticSymbolFilterModel(QObject *parent) : QSortFilterProxyModel(parent) {}

StaticSymbolModel *StaticSymbolFilterModel::castedSourceModel() {
  return dynamic_cast<StaticSymbolModel *>(sourceModel());
}

void StaticSymbolFilterModel::setSourceModel(QAbstractItemModel *sourceModel) {
  if (sourceModel == this->sourceModel()) return;
  auto old = castedSourceModel();
  if (old) disconnect(old, nullptr, this, nullptr);
  if (auto casted = dynamic_cast<StaticSymbolModel *>(sourceModel); casted != nullptr) {
    QSortFilterProxyModel::setSourceModel(casted);
    auto reset_model = [this]() {
      beginResetModel();
      endResetModel();
    };
    connect(casted, &StaticSymbolModel::longestChanged, this, &StaticSymbolFilterModel::longestChanged);
    connect(casted, &StaticSymbolModel::modelReset, this, reset_model);
    emit sourceModelChanged();
  }
}

QString StaticSymbolFilterModel::scopeFilter() const { return _scopeFilter; }

void StaticSymbolFilterModel::setScopeFilter(const QString &scopeFilter) {
  if (_scopeFilter == scopeFilter) return;
  _scopeFilter = scopeFilter;
  emit scopeFilterChanged();
  invalidateFilter();
}

qsizetype StaticSymbolFilterModel::longest() const {
  if (auto model = dynamic_cast<StaticSymbolModel *>(sourceModel()); model == nullptr) return 0;
  else return model->longest();
}

bool StaticSymbolFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
  auto sm = sourceModel();
  if (!sm) return false;
  auto index = sm->index(source_row, 0, source_parent);
  auto binding = sm->data(index, StaticSymbolModel::SymbolBindingRole).value<symbol::Binding>();
  if (binding == symbol::Binding::kGlobal) return true;
  auto scope = sm->data(index, StaticSymbolModel::ScopeRole).toString();
  return scope == _scopeFilter || _scopeFilter.isEmpty();
}

StaticSymbolReshapeModel::StaticSymbolReshapeModel(QObject *parent) : QAbstractProxyModel(parent) {}

StaticSymbolFilterModel *StaticSymbolReshapeModel::castedSourceModel() {
  return dynamic_cast<StaticSymbolFilterModel *>(sourceModel());
}

void StaticSymbolReshapeModel::setSourceModel(QAbstractItemModel *sourceModel) {
  if (sourceModel == this->sourceModel()) return;
  auto old = castedSourceModel();
  if (old) disconnect(old, nullptr, this, nullptr);

  if (auto casted = dynamic_cast<StaticSymbolFilterModel *>(sourceModel); casted != nullptr) {
    QAbstractProxyModel::setSourceModel(casted);
    auto reset_model = [this]() {
      beginResetModel();
      endResetModel();
    };
    connect(casted, &StaticSymbolFilterModel::longestChanged, this, &StaticSymbolReshapeModel::longestChanged);
    connect(casted, &StaticSymbolFilterModel::sourceModelChanged, this, reset_model);
    connect(casted, &StaticSymbolFilterModel::modelReset, this, reset_model);
    // When filters change, the entire outer model is invalidated.
    connect(casted, &StaticSymbolFilterModel::scopeFilterChanged, this, reset_model);
    emit sourceModelChanged();
  }
}

qsizetype StaticSymbolReshapeModel::longest() const {
  if (auto model = dynamic_cast<StaticSymbolFilterModel *>(sourceModel()); model == nullptr) return 0;
  else return model->longest();
}

void StaticSymbolReshapeModel::selectRectangle(QItemSelectionModel *selectionModel, const QModelIndex &topLeft,
                                               const QModelIndex &bottomRight) const {
  if (!selectionModel || !topLeft.isValid() || !bottomRight.isValid()) return;

  QItemSelection selection(topLeft, bottomRight);
  selectionModel->select(selection, QItemSelectionModel::Select | QItemSelectionModel::Clear);
}

void StaticSymbolReshapeModel::copy(const QList<QModelIndex> &indices) const {
  using namespace Qt::StringLiterals;
  if (indices.isEmpty()) return;
  else if (sourceModel() == nullptr) return;
  auto _longest = longest();
  auto numModelColumns = copyColumnCount() > 0 ? copyColumnCount() : columnCount({});
  auto colCount = qMin(numModelColumns, indices.size());
  int leftSize = qMax(_longest, 6), rightSize = 5, intraColPadding = 6;
  auto colPlaceholder = u"%1 %2"_s;
  auto colSpacer = u" "_s.repeated(intraColPadding);
  auto bar = u"-"_s.repeated(colCount * (leftSize + 1 + rightSize + intraColPadding) - intraColPadding);
  auto header = colPlaceholder.arg(u"Symbol"_s.leftJustified(leftSize, ' '), u"Value"_s.rightJustified(rightSize, ' '));
  // Rows contains the final (output) list of columns, and wip contains the row we are currently constructing.
  QStringList rows, wip;
  // From now on, wip must always have columnCount elements.
  for (int it = 0; it < colCount; it++) wip.append(header);

  // Create header, resetting wip to be empty.
  rows.append(bar);
  rows.append(wip.join(colSpacer));
  rows.append(bar);
  wip.fill("");

  // Track how many non-empty elements have been written to wip.
  int colIt = 0;
  // Write out each index to the wip buffer, flushing to rows when full.
  for (const auto &index : indices) {
    if (!index.isValid()) continue;

    auto symbol = data(index, StaticSymbolModel::SymbolRole).toString(),
         value = data(index, StaticSymbolModel::ValueRole).toString();
    wip[colIt++] = colPlaceholder.arg(symbol.leftJustified(leftSize, ' '), value.rightJustified(rightSize, ' '));

    if (colIt >= colCount) {
      rows.append(wip.join(colSpacer));
      colIt = 0;
      wip.fill("");
    }
  }
  // Write out the last partially-filled row if it exists and create the footer
  if (colIt > 0) rows.append(wip.join(colSpacer));
  rows.append(bar);

  // Only attempt clipboard access if the application has a clipboard.
  QClipboard *clipboard = QGuiApplication::clipboard();
  if (clipboard) clipboard->setText(rows.join("\n"));
}

int StaticSymbolReshapeModel::rowCount(const QModelIndex &parent) const {
  if (auto model = dynamic_cast<StaticSymbolFilterModel *>(sourceModel()); !model) return 0;
  else return (model->rowCount(mapToSource(parent)) + (_columnCount - 1)) / _columnCount;
}

int StaticSymbolReshapeModel::columnCount(const QModelIndex &parent) const { return _columnCount; }

int StaticSymbolReshapeModel::copyColumnCount() const { return _copyColumnCount; }

void StaticSymbolReshapeModel::setCopyColumnCount(int count) {
  if (count == _copyColumnCount) return;
  _copyColumnCount = count;
  emit copyColumnCountChanged();
}

void StaticSymbolReshapeModel::setColumnCount(int count) {
  if (count == _columnCount || count <= 0) return;
  beginResetModel();
  _columnCount = count;
  endResetModel();
  emit layoutChanged();
}

QModelIndex StaticSymbolReshapeModel::mapToSource(const QModelIndex &proxyIndex) const {
  if (!proxyIndex.isValid()) return {};

  int row = proxyIndex.row(), column = proxyIndex.column();
  int sourceRow = row * _columnCount + column;

  if (!sourceModel() || sourceRow >= sourceModel()->rowCount()) return {};
  return sourceModel()->index(sourceRow, 0, {});
}

QModelIndex StaticSymbolReshapeModel::mapFromSource(const QModelIndex &sourceIndex) const {
  if (!sourceIndex.isValid() || sourceIndex.column() != 0) return {};

  int sourceRow = sourceIndex.row();
  int row = sourceRow / _columnCount, column = sourceRow % _columnCount;
  return index(row, column, {});
}

QModelIndex StaticSymbolReshapeModel::index(int row, int column, const QModelIndex &parent) const {
  return createIndex(row, column);
}

// Flat 2D model, so no parent-children relationships.
QModelIndex StaticSymbolReshapeModel::parent(const QModelIndex &child) const { return {}; }

QVariant StaticSymbolReshapeModel::data(const QModelIndex &index, int role) const {
  if (auto source = sourceModel(); source == nullptr) return {};
  else if (!index.isValid()) return {};
  // Convert 2D-index back to 1D before deferring to old model
  else if (auto mapped = mapToSource(index); !mapped.isValid()) {
    if (role == (int)StaticSymbolModel::RoleNames::ValidRole) return false;
    return {};
  } else return source->data(mapped, role);
}
