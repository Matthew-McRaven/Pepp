#include "opcodemodel.hpp"
#include "enums/isa/pep10.hpp"
#include "enums/isa/pep9.hpp"
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
  if (index > 0 && index < _mnemonics.size()) return _mnemonics[index].opcode;
  return -1;
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
    case 4: return _rows[index.row()].status_bits; ;
    }
    break;
  }
  return {};
}

GreencardModel::Row from_mn(isa::detail::pep10::Mnemonic mn, QString bits = "") {
  using namespace isa::detail::pep10;
  using enum isa::Pep10::InstructionType;
  auto mn_str = isa::Pep10::string(mn);
  auto type = isa::Pep10::opcodeLUT[static_cast<quint8>(mn)].instr.type;
  // If instruction has register field, replace specific register with r
  if (type == RAAA_all || type == RAAA_noi || type == R_none) mn_str[mn_str.length() - 1] = 'r';

  return GreencardModel::Row{
      .sort_order = static_cast<quint8>(mn),
      .bit_pattern = isa::Pep10::instructionSpecifierWithPlaceholders(mn),
      .mnemonic = mn_str,
      .instruction = isa::Pep10::describeMnemonicUsingPlaceholders(mn),
      .addressing = "",
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

  return GreencardModel::Row{
      .sort_order = static_cast<quint8>(mn),
      .bit_pattern = isa::Pep9::instructionSpecifierWithPlaceholders(mn),
      .mnemonic = mn_str,
      .instruction = isa::Pep9::describeMnemonicUsingPlaceholders(mn),
      .addressing = "",
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
                         .bit_pattern = "00000000",
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
  _rows.emplace_back(from_mn(NEGX, "NZVC"));
  _rows.emplace_back(from_mn(ASLX, "NZVC"));
  _rows.emplace_back(from_mn(ASRX, "NZVC"));
  _rows.emplace_back(from_mn(NOTX, "NZ"));
  _rows.emplace_back(from_mn(ROLX, "NZC"));
  _rows.emplace_back(from_mn(RORX, "NZC"));
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
  _rows.emplace_back(from_mn(SCALL));
  _rows.emplace_back(from_mn(ADDSP));
  _rows.emplace_back(from_mn(SUBSP));
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
  _rows.emplace_back(from_mn(NOTX, "NZ"));
  _rows.emplace_back(from_mn(NEGX, "NZV"));
  _rows.emplace_back(from_mn(ASLX, "NZVC"));
  _rows.emplace_back(from_mn(ASRX, "NZC"));
  _rows.emplace_back(from_mn(ROLX, "C"));
  _rows.emplace_back(from_mn(RORX, "C"));
  _rows.emplace_back(from_mn(BR));
  _rows.emplace_back(from_mn(BRLE));
  _rows.emplace_back(from_mn(BRLT));
  _rows.emplace_back(from_mn(BREQ));
  _rows.emplace_back(from_mn(BRNE));
  _rows.emplace_back(from_mn(BRGE));
  _rows.emplace_back(from_mn(BRGT));
  _rows.emplace_back(from_mn(BRV));
  _rows.emplace_back(from_mn(BRC));

  _rows.emplace_back(from_mn(NOP0));
  _rows.emplace_back(from_mn(NOP));
  _rows.emplace_back(from_mn(DECI, "NZV"));
  _rows.emplace_back(from_mn(DECO));
  _rows.emplace_back(from_mn(HEXO));
  _rows.emplace_back(from_mn(STRO));

  _rows.emplace_back(from_mn(ADDSP, "NZVC"));
  _rows.emplace_back(from_mn(SUBSP, "NZVC"));

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
