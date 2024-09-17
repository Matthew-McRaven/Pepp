#include "symbolmodel.hpp"
#include "elfio/elfio.hpp"

SymbolModel::SymbolModel(QObject *parent) : QAbstractTableModel(parent) {}

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
  emit beginResetModel();
  _entries.clear();
  for (ELFIO::Elf_Xword index = 1; index < sym_access.get_symbols_num(); index++) {
    sym_access.get_symbol(index, name, value, size, bind, type, section_index, other);
    Entry e;
    e.name = QString::fromStdString(name);
    e.value = value;
    _entries.append(Entry{.name = QString::fromStdString(name), .value = value});
  }
  emit endResetModel();
}

void SymbolModel::clearData() {
  emit beginResetModel();
  _entries.clear();
  emit endResetModel();
}

int SymbolModel::rowCount(const QModelIndex &parent) const { return _entries.size(); }

int SymbolModel::columnCount(const QModelIndex &parent) const { return 2; }

QVariant SymbolModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) return QVariant();
  auto entry = _entries.at(index.row());
  switch (role) {
  case Qt::DisplayRole:
    if (index.column() == 0) return entry.name;
    else return QString::number(entry.value, 16);
    break;
  default: break;
  }
  return {};
}
