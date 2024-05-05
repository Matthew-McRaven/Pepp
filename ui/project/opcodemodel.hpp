#pragma once

#include <QAbstractListModel>

class OpcodeModel : public QAbstractListModel {
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
  // Store the opcode's mnemonic string and intege value.
  using T = std::pair<QString, quint8>;
  // Map index/rows to opcodes.
  using MapT = QMap<qsizetype, T>;
  MapT _mnemonics = {};
};
