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
  case Qt::DisplayRole: return item->format();
  case static_cast<int>(Roles::Box): return !item->readOnly();
  case static_cast<int>(Roles::RightJustify): return index.column() == 0;
  case static_cast<int>(Roles::Choices): {
    auto asChoice = dynamic_cast<ChoiceFormatter *>(item.data());
    if (asChoice) return asChoice->choices();
    return {};
  }
  case static_cast<int>(Roles::Selected): {
    auto asChoice = dynamic_cast<ChoiceFormatter *>(item.data());
    if (asChoice) return asChoice->currentIndex();
    return {};
  }
  }
  return {};
}

bool RegisterModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
  if (!index.isValid()) return false;
  const auto row = index.row();
  const auto col = index.column();
  auto item = _data[row][col];
  switch (role) {
  case static_cast<int>(Roles::Selected): {
    auto asChoice = dynamic_cast<ChoiceFormatter *>(item.data());
    if (asChoice) {
      asChoice->setCurrentIndex(value.toInt());
      onUpdateGUI(); // Force full-screen refresh, since the width of all rows & columns changed.
      return true;
    }
    return false;
  }
  default: return false;
  }
}

Qt::ItemFlags RegisterModel::flags(const QModelIndex &index) const
{
  if (!index.isValid()) return {};
  const auto row = index.row();
  const auto col = index.column();
  auto item = _data[row][col];
  auto asChoice = dynamic_cast<ChoiceFormatter *>(item.data());
  if (!asChoice) return Qt::ItemIsEnabled;
  return Qt::ItemIsEnabled | Qt::ItemIsEditable;
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

void RegisterModel::onUpdateGUI() {
  beginResetModel();
  endResetModel();
}

QHash<int, QByteArray> RegisterModel::roleNames() const {
  static const QHash<int, QByteArray> ret{{Qt::DisplayRole, "display"},
                                          {static_cast<int>(Roles::Box), "box"},
                                          {static_cast<int>(Roles::RightJustify), "rightJustify"},
                                          {static_cast<int>(Roles::Choices), "choices"},
                                          {static_cast<int>(Roles::Selected), "selected"}};
  return ret;
}
