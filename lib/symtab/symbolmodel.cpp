#include "symbolmodel.hpp"
#include "elfio/elfio.hpp"

SymbolModel::SymbolModel(QObject *parent) : QAbstractListModel(parent) {
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
  }

  qDebug() << "Longest: " << longest_;

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

int SymbolModel::rowCount(const QModelIndex &parent) const { return entries_.size(); }

QVariant SymbolModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) return QVariant();
  auto entry = entries_.at(index.row());
  switch (role) {
  case SymbolRole: return entry.name;
  case ValueRole: return QStringLiteral("%1").arg(entry.value, 4, 16, QLatin1Char('0')).toUpper();
  case IndexRole: return index.row();
  default: break;
  }
  return {};
}

QHash<int, QByteArray> SymbolModel::roleNames() const { return roleNames_; }
