#pragma once
#include <QSortFilterProxyModel>
#include <QTransposeProxyModel>
#include <qqmlintegration.h>

class RowFilterModel : public QSortFilterProxyModel {
  Q_OBJECT
  Q_PROPERTY(int row READ row WRITE setRow NOTIFY rowChanged)
  QML_NAMED_ELEMENT(RowFilterModel);

public:
  RowFilterModel(QObject *parent = nullptr);
  int row() const;
  void setRow(int row);

signals:
  void rowChanged();

protected:
  bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
  int _row = -1;
};

struct ForeginQTransposePrQML_FOREGINxyModel {
  Q_GADGET
  QML_FOREIGN(QTransposeProxyModel);
  QML_NAMED_ELEMENT(TransposeProxyModel);
};
