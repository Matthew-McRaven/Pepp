#include "registermodel.hpp"

//  For testing only
#include <QRandomGenerator>

RegisterModel::RegisterModel(QObject *parent) : QAbstractTableModel(parent) {}

int RegisterModel::rowCount(const QModelIndex &) const {
  return _data.length();
}

int RegisterModel::columnCount(const QModelIndex &parent) const {
  return _cols;
}

QVariant RegisterModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
      return {};

    const auto row = index.row();
    const auto col = index.column();
    auto item = _data[row][col];
    switch (role) {
    case static_cast<int>(Roles::ReadOnly):
      return item->readOnly();
      break;
    case Qt::DisplayRole:
      return item->format();
      break;
    }
    return {};
}

bool RegisterModel::setData(const QModelIndex &index, const QVariant &value, int role) { return false; }

void RegisterModel::appendFormatters(QVector<QSharedPointer<RegisterFormatter>> formatters) {
  beginResetModel();
  _cols = std::max(_cols, static_cast<quint32>(formatters.length()));
  _data.append(formatters);
  endResetModel();
}

qsizetype RegisterModel::columnCharWidth(int column) const {
  qsizetype ret = 0;
  for (const auto &row : _data) {
    ret = std::max(ret, row[column]->length());
  }
  return ret;
}

QHash<int, QByteArray> RegisterModel::roleNames() const {
  static const QHash<int, QByteArray> ret{{Qt::DisplayRole, "display"},
                                          {static_cast<int>(Roles::ReadOnly), "readOnly"}};
  return ret;
}
