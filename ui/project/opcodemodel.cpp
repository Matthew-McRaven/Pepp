#include "opcodemodel.hpp"
#include "isa/pep10.hpp"
Pep10OpcodeModel::Pep10OpcodeModel(QObject *parent) : QAbstractListModel(parent) {
  static const auto mnemonicEnum = QMetaEnum::fromType<isa::Pep10::Mnemonic>();
  static const auto addressEnum = QMetaEnum::fromType<isa::Pep10::AddressingMode>();
  for (int it = 0; it < 256; it++) {
    auto op = isa::Pep10::opcodeLUT[it];
    if (!op.valid)
      continue;
    QString formatted;
    if (op.instr.unary) {
      formatted = QString(mnemonicEnum.valueToKey((int)op.instr.mnemon)).toUpper();
    } else {
      formatted = u"%1, %2"_qs.arg(QString(mnemonicEnum.valueToKey((int)op.instr.mnemon)).toUpper(),
                                   QString(addressEnum.valueToKey((int)op.mode)).toLower());
    }
    _mnemonics[_mnemonics.size()] = {formatted, it};
  }
}

int Pep10OpcodeModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return 0;
  return _mnemonics.size();
}

QVariant Pep10OpcodeModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();
  switch (role) {
  case Qt::DisplayRole:
    return _mnemonics[index.row()].first;
  }
  return {};
}

qsizetype Pep10OpcodeModel::indexFromOpcode(quint8 opcode) const {

  auto result =
      std::find_if(_mnemonics.begin(), _mnemonics.end(), [&](const auto &pair) { return pair.second == opcode; });
  if (result == _mnemonics.end())
    return 0;
  return result.key();
}

quint8 Pep10OpcodeModel::opcodeFromIndex(qsizetype index) const {
  if (_mnemonics.contains(index))
    return _mnemonics[index].second;
  return -1;
}
