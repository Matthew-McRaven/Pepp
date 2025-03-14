#include "symbolmodel.hpp"
#include <QClipboard>
#include <QGuiApplication>
#include <QItemSelection>
#include <QItemSelectionModel>
#include "elfio/elfio.hpp"

SymbolModel::SymbolModel(QObject *parent) : QAbstractTableModel(parent) {
  //  List column names that will appear in view
  roleNames_[SymbolRole] = "symbol";
  roleNames_[ValueRole] = "value";
  roleNames_[IndexRole] = "index";
}

void SymbolModel::setFromElf(ELFIO::elfio *elf, QString tableSection) {
  auto t = tableSection.toStdString();
  ELFIO::section *table = nullptr;
  for (const auto &section : elf->sections) {
    if (section->get_name() == t) table = section.get();
  }
  if (!table) return;

  std::string name;
  ELFIO::Elf64_Addr value;
  ELFIO::Elf_Xword size;
  unsigned char bind;
  unsigned char type;
  ELFIO::Elf_Half section_index;
  unsigned char other;

  ELFIO::symbol_section_accessor sym_access(*elf, table);
  beginResetModel();
  entries_.clear();
  for (ELFIO::Elf_Xword index = 1; index < sym_access.get_symbols_num(); index++) {
    sym_access.get_symbol(index, name, value, size, bind, type, section_index, other);
    Entry e;
    e.name = QString::fromStdString(name);
    e.value = value;
    entries_.append(Entry{.name = QString::fromStdString(name), .value = value});

    //  Find longest key
    longest_ = std::max(longest_, e.name.length());
    emit longestChanged();
  }

  //  Sort list, case insensitive
  std::sort(entries_.begin(), entries_.end(), [&](const Entry &s1, const Entry &s2) {
    return QString::compare(s1.name, s2.name, Qt::CaseInsensitive) < 0;
  });
  endResetModel();
}

void SymbolModel::clearData() {
  beginResetModel();
  entries_.clear();
  longest_ = 0;
  endResetModel();
}

int SymbolModel::rowCount(const QModelIndex &parent) const {
  return (entries_.size() + (_columnCount - 1)) / _columnCount;
}

int SymbolModel::columnCount(const QModelIndex &parent) const { return _columnCount; }

void SymbolModel::setColumnCount(int count) {
  if (count == _columnCount || count <= 0) return;
  _columnCount = count;
  emit layoutChanged();
}

QVariant SymbolModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) return QVariant();
  auto offset = index.row() * _columnCount + index.column();

  if (offset >= entries_.size()) {
    if (role == IndexRole) return -1;
    else return "";
  }

  auto entry = entries_.at(offset);
  switch (role) {
  case SymbolRole: return entry.name;
  case ValueRole: return QStringLiteral("%1").arg(entry.value, 4, 16, QLatin1Char('0')).toUpper();
  case IndexRole: return index.row();
  default: break;
  }
  return {};
}

Qt::ItemFlags SymbolModel::flags(const QModelIndex &index) const { return Qt::ItemIsEnabled | Qt::ItemIsSelectable; }

void SymbolModel::selectRectangle(QItemSelectionModel *selectionModel, const QModelIndex &topLeft,
                                  const QModelIndex &bottomRight) const {
  if (!selectionModel || !topLeft.isValid() || !bottomRight.isValid()) return;

  QItemSelection selection(topLeft, bottomRight);
  selectionModel->select(selection, QItemSelectionModel::Select | QItemSelectionModel::Clear);
}

using namespace Qt::StringLiterals;
void SymbolModel::copy(const QList<QModelIndex> &indices) const {
  if (indices.isEmpty()) return;
  auto colCount = qMin(columnCount({}), indices.size());
  int leftSize = qMax(longest_, 6), rightSize = 5, intraColPadding = 6;
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

    auto symbol = data(index, SymbolRole).toString(), value = data(index, ValueRole).toString();
    wip[colIt++] = colPlaceholder.arg(symbol.leftJustified(leftSize, ' '), value.rightJustified(rightSize, ' '));

    if (colIt >= colCount) {
      rows.append(wip.join(colSpacer));
      colIt = 0;
      wip.fill("");
    }
  }
  // Write out the last partially-filled row if it exists and create the footer

  rows.append(bar);

  // Only attempt clipboard access if the application has a clipboard.
  QClipboard *clipboard = QGuiApplication::clipboard();
  if (clipboard) clipboard->setText(rows.join("\n"));
}

QHash<int, QByteArray> SymbolModel::roleNames() const { return roleNames_; };
