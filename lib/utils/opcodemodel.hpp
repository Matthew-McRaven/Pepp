#pragma once

#include <QAbstractListModel>
#include <QtCore>
#include <QtQmlIntegration>
#include "enums/constants.hpp"

class OpcodeModel : public QAbstractListModel {
  Q_OBJECT
  QML_ELEMENT

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

class GreencardModel : public QAbstractTableModel {
  Q_OBJECT
  QML_ELEMENT

public:
  enum class Roles { UseMonoRole = Qt::UserRole + 1, UseMarkdown };
  Q_ENUM(Roles);
  struct Row {
    quint8 sort_order;
    QString bit_pattern;
    QString mnemonic;
    QString instruction;
    QString addressing;
    QString status_bits;
  };
  explicit GreencardModel(QObject *parent = nullptr);

  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  Q_INVOKABLE void make_pep10();
  Q_INVOKABLE void make_pep9();

  pepp::Architecture arch() const { return _arch; }

private:
  std::vector<Row> _rows = {};
  pepp::Architecture _arch = pepp::Architecture::NO_ARCH;
};

class GreencardFilterModel : public QSortFilterProxyModel {
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(bool hideStatus READ hideStatus WRITE setHideStatus NOTIFY hideStatusChanged)
  Q_PROPERTY(bool hideMnemonic READ hideMnemonic WRITE setHideMnemonic NOTIFY hideMnemonicChanged)
  Q_PROPERTY(bool dyadicAddressing READ dyadicAddressing WRITE setDyadicAddressing NOTIFY dyadicAddressingChanged)
public:
  explicit GreencardFilterModel(QObject *parent = nullptr);
  void setSourceModel(QAbstractItemModel *sourceModel) override;
  bool hideStatus() const;
  void setHideStatus(bool hide);
  bool hideMnemonic() const;
  void setHideMnemonic(bool hide);
  bool dyadicAddressing() const;
  void setDyadicAddressing(bool simplify);

  QVariant data(const QModelIndex &index, int role) const override;
signals:
  void hideStatusChanged();
  void hideMnemonicChanged();
  void dyadicAddressingChanged();

protected:
  bool filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const override;

private:
  bool _hideStatus = false;
  bool _hideMnemonic = false;
  bool _dyadicAddressing = false;
};
