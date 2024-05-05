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
    return _mnemonics[index.row()].first;
  }
  return {};
}

qsizetype OpcodeModel::indexFromOpcode(quint8 opcode) const {

  auto result =
      std::find_if(_mnemonics.begin(), _mnemonics.end(), [&](const auto &pair) { return pair.second == opcode; });
  if (result == _mnemonics.end())
    return 0;
  return result.key();
}

quint8 OpcodeModel::opcodeFromIndex(qsizetype index) const {
  if (_mnemonics.contains(index))
    return _mnemonics[index].second;
  return -1;
}

void OpcodeModel::appendRow(QString mnemonic, quint8 opcode) { _mnemonics[_mnemonics.size()] = {mnemonic, opcode}; }
