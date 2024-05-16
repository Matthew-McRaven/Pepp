#include "opcodemodel.hpp"
OpcodeModel::OpcodeModel(QObject *parent) : QAbstractListModel(parent) {}

int OpcodeModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return _mnemonics.size();
}

QVariant OpcodeModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();
  switch (role) {
  case Qt::DisplayRole:
    return _mnemonics[index.row()].mnemonic_addr;
  }
  return {};
}

qsizetype OpcodeModel::indexFromOpcode(quint8 opcode) const {
  auto result =
      std::find_if(_mnemonics.begin(), _mnemonics.end(), [&](const auto &pair) { return pair.opcode == opcode; });
  if (result == _mnemonics.end())
    return -1;
  return result - _mnemonics.begin();
}

quint8 OpcodeModel::opcodeFromIndex(qsizetype index) const {
  if (index > 0 && index < _mnemonics.size())
    return _mnemonics[index].opcode;
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
        if (pair.mnemonic_only == toInsert.mnemonic_only)
          return pair.opcode < toInsert.opcode;
        return pair.mnemonic_only < toInsert.mnemonic_only;
      });
  _mnemonics.insert(index, toInsert);
}
