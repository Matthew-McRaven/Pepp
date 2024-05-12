#pragma once

#include <QAbstractListModel>
#include "utils_global.hpp"

class UTILS_EXPORT OpcodeModel : public QAbstractListModel {
  Q_OBJECT

public:
  explicit OpcodeModel(QObject *parent = nullptr);

  // Basic functionality:
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;

  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

  // Returns a row in the model if it maps to an opcode, or -1 if not.
  Q_INVOKABLE qsizetype indexFromOpcode(quint8 opcode) const;
  Q_INVOKABLE quint8 opcodeFromIndex(qsizetype index) const;

  void appendRow(QString mnemonic, quint8 opcode);

private:
  struct Opcode {
    quint8 opcode;
    QString mnemonic_addr;
    QStringView mnemonic_only;
  };
  // Map index/rows to opcodes. Store the opcode's mnemonic string and integer value.
  // Upon each insert, ensure that vector is sorted alphabetically by opcode name, then by opcode value for addressing
  // modes. The opcode value sorting is required to prevent ADDA,d from occuring before ADDA,i.
  // NOTE: I don't expect to insert more than 200 items, so the
  // N^2 performance of repeated insertion should be acceptable. This assumption will likely be broken for RISC-V, but I
  // will deal with that later.
  std::vector<Opcode> _mnemonics = {};
};
