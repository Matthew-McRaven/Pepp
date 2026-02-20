#pragma once

#include <QAbstractTableModel>
#include <qqmlintegration.h>
#include "core/ds/linenumbers.hpp"
#include "core/langs/ucode/ir_variant.hpp"

class Microcode : public QObject {
  Q_OBJECT
  QML_UNCREATABLE("Only create from C++");

public:
  explicit Microcode(pepp::MicrocodeChoice mc, pepp::Line2Address l2a, QObject *parent = nullptr);
  const pepp::MicrocodeChoice choice;
  const pepp::Line2Address line2addr;
};

namespace pepp {
class MicroObjectModel : public QAbstractTableModel {
  Q_OBJECT
  QML_NAMED_ELEMENT(MicroObjectModel)
  Q_PROPERTY(Microcode *microcode WRITE setMicrocode)
public:
  MicroObjectModel(QObject *parent = nullptr);
  int rowCount(const QModelIndex &parent) const override;
  int columnCount(const QModelIndex &parent) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
  QVariant data(const QModelIndex &index, int role) const override;
  void setMicrocode(Microcode *microcode);

private:
  // Both _headers and _values are copied from a given microcode program, eliminating dependence of the model on
  // microcode fields. Contains 1 extra column, which is the line number
  QStringList _headers;
  // A cell with -1 should be rendered as empty.
  // _values are copied from  a microcode program
  QList<QList<int>> _values;
};
} // namespace pepp
