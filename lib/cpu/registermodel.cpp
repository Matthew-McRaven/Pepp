#include "registermodel.hpp"
#include "./formats.hpp"
//  For testing only
#include <QRandomGenerator>

RegisterModel::RegisterModel(QObject *parent) : QAbstractTableModel(parent) {}

int RegisterModel::rowCount(const QModelIndex &) const { return _data.length(); }

int RegisterModel::columnCount(const QModelIndex &parent) const { return _cols; }

QVariant RegisterModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) return {};

  const auto row = index.row();
  const auto col = index.column();
  auto item = _data[row][col];
  switch (role) {
  case static_cast<int>(Roles::Box): return !item->readOnly();
  case static_cast<int>(Roles::RightJustify): return index.column() == 0;
  case Qt::DisplayRole: return item->format();
  }
  return {};
}

void RegisterModel::appendFormatters(QVector<QSharedPointer<RegisterFormatter>> formatters) {
  beginResetModel();
  _cols = std::max(_cols, static_cast<quint32>(formatters.length()));
  _data.append(formatters);
  endResetModel();
}

qsizetype RegisterModel::columnCharWidth(int column) const {
  if (column >= _cols) return 0;
  qsizetype ret = 0;
  for (const auto &row : _data) {
    auto rowMax = row[column]->length();
    ret = std::max(ret, rowMax);
  }
  return ret;
}

void RegisterModel::onUpdateGUI(sim::api2::trace::FrameIterator) {
  beginResetModel();
  endResetModel();
}

QHash<int, QByteArray> RegisterModel::roleNames() const {
  static const QHash<int, QByteArray> ret{{Qt::DisplayRole, "display"},
                                          {static_cast<int>(Roles::Box), "box"},
                                          {static_cast<int>(Roles::RightJustify), "rightJustify"}};
  return ret;
}
