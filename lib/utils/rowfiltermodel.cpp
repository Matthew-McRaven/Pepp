#include "rowfiltermodel.hpp"

RowFilterModel::RowFilterModel(QObject *parent) : QSortFilterProxyModel(parent) {}

int RowFilterModel::row() const { return _row; }

void RowFilterModel::setRow(int row) {
  if (row == _row) return;
  _row = row;
  emit rowChanged();
  invalidateFilter();
}

bool RowFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
  return source_row == _row;
}
