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
#include "opcodemodel.hpp"
#include "bts/isa/pep/pep10.hpp"
#include "bts/isa/pep/pep9.hpp"
OpcodeModel::OpcodeModel(QObject *parent) : QAbstractListModel(parent) {}

int OpcodeModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) return 0;
  return _mnemonics.size();
}

QVariant OpcodeModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) return QVariant();
  switch (role) {
  case Qt::DisplayRole: return _mnemonics[index.row()].mnemonic_addr;
  }
  return {};
}

qsizetype OpcodeModel::indexFromOpcode(quint8 opcode) const {
  auto result =
      std::find_if(_mnemonics.begin(), _mnemonics.end(), [&](const auto &pair) { return pair.opcode == opcode; });
  if (result == _mnemonics.end()) return -1;
  return result - _mnemonics.begin();
}

quint8 OpcodeModel::opcodeFromIndex(qsizetype index) const {
  if (index < 0 || index >= _mnemonics.size()) return -1;
  else return _mnemonics[index].opcode;
}

void OpcodeModel::appendRow(QString mnemonic, quint8 opcode) {
  auto indexOfComma = mnemonic.indexOf(',');
  Opcode toInsert{
      .opcode = opcode,
      .mnemonic_addr = mnemonic,
      .mnemonic_only = indexOfComma == -1 ? QStringView(mnemonic) : QStringView(mnemonic).left(indexOfComma),
  };
  // Sort the vector alphabetically on mnemonic, and on opcode for addressing modes.
  auto index =
      std::lower_bound(_mnemonics.begin(), _mnemonics.end(), toInsert, [](const auto &pair, const auto &toInsert) {
        if (pair.mnemonic_only == toInsert.mnemonic_only) return pair.opcode < toInsert.opcode;
        return pair.mnemonic_only < toInsert.mnemonic_only;
      });
  _mnemonics.insert(index, toInsert);
}

GreencardModel::GreencardModel(QObject *parent) : QAbstractTableModel(parent) {}

int GreencardModel::columnCount(const QModelIndex &parent) const { return 5; }

int GreencardModel::rowCount(const QModelIndex &parent) const { return _rows.size(); }

QVariant GreencardModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) return QVariant();
  switch (role) {
  case Qt::DisplayRole:
    switch (index.column()) {
    case 0: return _rows[index.row()].bit_pattern;
    case 1: return _rows[index.row()].mnemonic;
    case 2: return _rows[index.row()].instruction;
    case 3: return _rows[index.row()].addressing;
    case 4: return _rows[index.row()].status_bits;
    }
    break;
  case (int)Roles::UseMonoRole: return index.column() != 2;
  case (int)Roles::UseMarkdown: return index.column() == 2;
  }

  return {};
}

QVariant GreencardModel::headerData(int section, Qt::Orientation orientation, int role) const {
  switch (role) {
  case Qt::DisplayRole:
    switch (section) {
    case 0: return "Instruction Specifier";
    case 1: return "Mnemonic";
    case 2: return "Instruction";
    case 3: return "Addressing Modes";
    case 4: return "Status Bits";
    }
    break;
  }
  return {};
}

QHash<int, QByteArray> GreencardModel::roleNames() const {
  using enum Roles;
  static const auto ret = QHash<int, QByteArray>{
      {(int)Qt::DisplayRole, "display"},
      {(int)UseMonoRole, "useMonoRole"},
      {(int)UseMarkdown, "useMarkdown"},
  };
  return ret;
}
GreencardModel::Row blank() {
  return GreencardModel::Row{
      .sort_order = 255,
      .bit_pattern = "",
      .mnemonic = "",
      .instruction = "",
      .addressing = "",
      .status_bits = "",
  };
}
GreencardModel::Row from_mn(isa::detail::pep10::Mnemonic mn, QString bits = "") {
  using namespace isa::detail::pep10;
  using enum isa::Pep10::InstructionType;
  auto mn_str = isa::Pep10::string(mn);
  auto type = isa::Pep10::opcodeLUT[static_cast<quint8>(mn)].instr.type;
  // If instruction has register field, replace specific register with r
  if (type == RAAA_all || type == RAAA_noi || type == R_none) mn_str[mn_str.length() - 1] = 'r';
  QString addr_modes;
  switch (type) {
  case Invalid: [[fallthrough]];
  case U_none: [[fallthrough]];
  case R_none: addr_modes = "Monadic"; break;
  case A_ix: addr_modes = "i,x"; break;
  case AAA_i: addr_modes = "i"; break;
  case AAA_all: [[fallthrough]];
  case RAAA_all: addr_modes = "i,d,n,s,sf,x,sx,sfx"; break;
  case RAAA_noi: addr_modes = "d,n,s,sf,x,sx,sfx"; break;
  }
  auto is_bits = isa::Pep10::instructionSpecifierWithPlaceholders(mn);
  return GreencardModel::Row{
      .sort_order = static_cast<quint8>(mn),
      .bit_pattern = is_bits.left(4) + " " + is_bits.right(4),
      .mnemonic = mn_str,
      .instruction = isa::Pep10::describeMnemonicUsingPlaceholders(mn),
      .addressing = addr_modes,
      .status_bits = bits,
  };
}

GreencardModel::Row from_mn(isa::detail::pep9::Mnemonic mn, QString bits = "") {
  using namespace isa::detail::pep9;
  using enum isa::Pep9::InstructionType;
  auto mn_str = isa::Pep9::string(mn);
  auto type = isa::Pep9::opcodeLUT[static_cast<quint8>(mn)].instr.type;
  // If instruction has register field, replace specific register with r
  if (type == RAAA_all || type == RAAA_noi || type == R_none) mn_str[mn_str.length() - 1] = 'r';
  else if (type == N_none) mn_str[mn_str.length() - 1] = 'n';
  QString addr_modes;
  switch (type) {
  case Invalid: [[fallthrough]];
  case U_none: [[fallthrough]];
  case N_none: [[fallthrough]];
  case R_none: addr_modes = "U"; break;
  case A_ix: addr_modes = "i,x"; break;
  case AAA_i: addr_modes = "i"; break;
  case AAA_all: [[fallthrough]];
  case RAAA_all: addr_modes = "i,d,n,s,sf,x,sx,sfx"; break;
  case AAA_noi: [[fallthrough]];
  case RAAA_noi: addr_modes = "d,n,s,sf,x,sx,sfx"; break;
  case AAA_stro: addr_modes = "d,n,s,sf,x"; break;
  }
  auto is_bits = isa::Pep9::instructionSpecifierWithPlaceholders(mn);
  return GreencardModel::Row{
      .sort_order = static_cast<quint8>(mn),
      .bit_pattern = is_bits.left(4) + " " + is_bits.right(4),
      .mnemonic = mn_str,
      .instruction = isa::Pep9::describeMnemonicUsingPlaceholders(mn),
      .addressing = addr_modes,
      .status_bits = bits,
  };
}

void GreencardModel::make_pep10() {
  using enum isa::detail::pep10::Mnemonic;
  if (_arch == pepp::Architecture::PEP10) return;

  beginResetModel();
  _arch = pepp::Architecture::PEP10;
  _rows.clear();
  _rows.emplace_back(Row{.sort_order = 0,
                         .bit_pattern = "0000 0000",
                         .mnemonic = "",
                         .instruction = "Illegal Instruction",
                         .addressing = "",
                         .status_bits = ""});
  _rows.emplace_back(from_mn(RET));
  _rows.emplace_back(from_mn(SRET));
  _rows.emplace_back(from_mn(MOVFLGA));
  _rows.emplace_back(from_mn(MOVAFLG, "NZVC"));
  _rows.emplace_back(from_mn(MOVSPA));
  _rows.emplace_back(from_mn(MOVASP));
  _rows.emplace_back(from_mn(NOP));
  _rows.emplace_back(blank());

  _rows.emplace_back(from_mn(NEGX, "NZVC"));
  _rows.emplace_back(from_mn(ASLX, "NZVC"));
  _rows.emplace_back(from_mn(ASRX, "NZVC"));
  _rows.emplace_back(from_mn(NOTX, "NZ"));
  _rows.emplace_back(from_mn(ROLX, "NZC"));
  _rows.emplace_back(from_mn(RORX, "NZC"));
  _rows.emplace_back(blank());

  _rows.emplace_back(from_mn(BR));
  _rows.emplace_back(from_mn(BRLE));
  _rows.emplace_back(from_mn(BRLT));
  _rows.emplace_back(from_mn(BREQ));
  _rows.emplace_back(from_mn(BRNE));
  _rows.emplace_back(from_mn(BRGE));
  _rows.emplace_back(from_mn(BRGT));
  _rows.emplace_back(from_mn(BRV));
  _rows.emplace_back(from_mn(BRC));
  _rows.emplace_back(from_mn(CALL));
  _rows.emplace_back(blank());

  _rows.emplace_back(from_mn(SCALL));
  _rows.emplace_back(from_mn(ADDSP));
  _rows.emplace_back(from_mn(SUBSP));
  _rows.emplace_back(blank());

  _rows.emplace_back(from_mn(ADDX, "NZVC"));
  _rows.emplace_back(from_mn(SUBX, "NZVC"));
  _rows.emplace_back(from_mn(ANDX, "NZ"));
  _rows.emplace_back(from_mn(ORX, "NZ"));
  _rows.emplace_back(from_mn(XORX, "NZ"));
  _rows.emplace_back(from_mn(CPWX, "NZVC"));
  _rows.emplace_back(from_mn(CPBX, "NZVC"));
  _rows.emplace_back(from_mn(LDWX, "NZ"));
  _rows.emplace_back(from_mn(LDBX, "NZ"));
  _rows.emplace_back(from_mn(STWX));
  _rows.emplace_back(from_mn(STBX));
  endResetModel();
}

void GreencardModel::make_pep9() {
  using enum isa::detail::pep9::Mnemonic;
  if (_arch == pepp::Architecture::PEP9) return;

  beginResetModel();
  _arch = pepp::Architecture::PEP9;
  _rows.clear();
  _rows.emplace_back(from_mn(STOP));
  _rows.emplace_back(from_mn(RET));
  _rows.emplace_back(from_mn(RETTR));
  _rows.emplace_back(from_mn(MOVSPA));
  _rows.emplace_back(from_mn(MOVFLGA));
  _rows.emplace_back(from_mn(MOVAFLG, "NZVC"));
  _rows.emplace_back(blank());

  _rows.emplace_back(from_mn(NOTX, "NZ"));
  _rows.emplace_back(from_mn(NEGX, "NZV"));
  _rows.emplace_back(from_mn(ASLX, "NZVC"));
  _rows.emplace_back(from_mn(ASRX, "NZC"));
  _rows.emplace_back(from_mn(ROLX, "C"));
  _rows.emplace_back(from_mn(RORX, "C"));
  _rows.emplace_back(blank());

  _rows.emplace_back(from_mn(BR));
  _rows.emplace_back(from_mn(BRLE));
  _rows.emplace_back(from_mn(BRLT));
  _rows.emplace_back(from_mn(BREQ));
  _rows.emplace_back(from_mn(BRNE));
  _rows.emplace_back(from_mn(BRGE));
  _rows.emplace_back(from_mn(BRGT));
  _rows.emplace_back(from_mn(BRV));
  _rows.emplace_back(from_mn(BRC));
  _rows.emplace_back(blank());

  _rows.emplace_back(from_mn(NOP0));
  _rows.emplace_back(from_mn(NOP));
  _rows.emplace_back(from_mn(DECI, "NZV"));
  _rows.emplace_back(from_mn(DECO));
  _rows.emplace_back(from_mn(HEXO));
  _rows.emplace_back(from_mn(STRO));
  _rows.emplace_back(blank());

  _rows.emplace_back(from_mn(ADDSP, "NZVC"));
  _rows.emplace_back(from_mn(SUBSP, "NZVC"));
  _rows.emplace_back(blank());

  _rows.emplace_back(from_mn(ADDX, "NZVC"));
  _rows.emplace_back(from_mn(SUBX, "NZVC"));
  _rows.emplace_back(from_mn(ANDX, "NZ"));
  _rows.emplace_back(from_mn(ORX, "NZ"));
  _rows.emplace_back(from_mn(CPWX, "NZVC"));
  _rows.emplace_back(from_mn(CPBX, "NZVC"));
  _rows.emplace_back(from_mn(LDWX, "NZ"));
  _rows.emplace_back(from_mn(LDBX, "NZ"));
  _rows.emplace_back(from_mn(STWX));
  _rows.emplace_back(from_mn(STBX));
  endResetModel();
}

GreencardFilterModel::GreencardFilterModel(QObject *parent) : QSortFilterProxyModel(parent) {}

void GreencardFilterModel::setSourceModel(QAbstractItemModel *sourceModel) {
  if (this->sourceModel() == sourceModel) return;
  else if (auto casted = qobject_cast<GreencardModel *>(sourceModel); casted == nullptr)
    qFatal("GreencardFilterModel only accepts GreencardModel as sourceModel");
  QSortFilterProxyModel::setSourceModel(sourceModel);
}

bool GreencardFilterModel::hideStatus() const { return _hideStatus; }

void GreencardFilterModel::setHideStatus(bool hide) {
  if (hide == _hideStatus) return;
  _hideStatus = hide;
  emit hideStatusChanged();
  invalidateFilter();
}

bool GreencardFilterModel::hideMnemonic() const { return _hideMnemonic; }

void GreencardFilterModel::setHideMnemonic(bool hide) {
  if (hide == _hideMnemonic) return;
  _hideMnemonic = hide;
  emit hideMnemonicChanged();
  invalidateFilter();
}

bool GreencardFilterModel::dyadicAddressing() const { return _dyadicAddressing; }

void GreencardFilterModel::setDyadicAddressing(bool simplify) {
  if (simplify == _dyadicAddressing) return;
  _dyadicAddressing = simplify;
  emit dyadicAddressingChanged();
  invalidateFilter();
}

QVariant GreencardFilterModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) return QVariant();
  else if (auto d = QSortFilterProxyModel::data(index, role); !d.isValid()) return d;
  // If column == addressing, and dyadicAddressing is true, return "monadic" if that was the value in that column, else
  // return dyadic. Otherwise, return the data from the source model.
  else if (auto sourceIndex = mapToSource(index);
           sourceIndex.isValid() && sourceIndex.column() == 3 && role == Qt::DisplayRole && _dyadicAddressing) {
    auto data = d.toString().toLower();
    if (data == "" || data == "monadic" || data == "u") return d;
    else if (auto casted = qobject_cast<GreencardModel *>(sourceModel());
             casted && casted->arch() == pepp::Architecture::PEP10)
      return "Dyadic";
    else return "Nonunary";
  } else return d;
}

bool GreencardFilterModel::filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const {
  if (source_column == 1 && _hideMnemonic) return false;
  if (source_column == 4 && _hideStatus) return false;
  return true;
}
