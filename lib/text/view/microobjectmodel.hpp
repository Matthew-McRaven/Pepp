#pragma once

#include <QAbstractTableModel>
#include <qqmlintegration.h>

namespace pepp {
class MicroObjectModel : public QAbstractTableModel {
  Q_OBJECT
  QML_NAMED_ELEMENT(MicroObjectModel)
public:
  MicroObjectModel(QObject *parent = nullptr);
  int rowCount(const QModelIndex &parent) const override;
  int columnCount(const QModelIndex &parent) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
  QVariant data(const QModelIndex &index, int role) const override;

private:
  // Both _headers and _values are copied from a given microcode program, eliminating dependence of the model on
  // microcode fields.
  QStringList _headers;
  // A cell with -1 should be rendered as empty.
  // _values are copied from  a microcode program
  QList<QList<int>> _values;
};
} // namespace pepp
