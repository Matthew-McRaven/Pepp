#include "statusbitmodel.hpp"

Flag::Flag(QString name, std::function<bool()> value) : _name(name), _fn(value) {}

QString Flag::name() const { return _name; }

bool Flag::value() const { return _fn(); }

FlagModel::FlagModel(QObject *parent) : QAbstractListModel(parent) {}

int FlagModel::rowCount(const QModelIndex &) const { return _flags.size(); }

QVariant FlagModel::data(const QModelIndex &index, int role) const {
  const auto row = index.row();
  if (!index.isValid() || row < 0 || row >= _flags.size()) return {};
  auto flag = _flags[row];

  switch (role) {
  case Qt::DisplayRole: return flag->name();
  case static_cast<int>(Roles::Value): return flag->value();
  }
  return {};
}

void FlagModel::appendFlag(QSharedPointer<Flag> flag) {
  beginResetModel();
  _flags.append(flag);
  endResetModel();
}

void FlagModel::onUpdateGUI(sim::api2::trace::FrameIterator) {
  beginResetModel();
  endResetModel();
}

QHash<int, QByteArray> FlagModel::roleNames() const {
  static QHash<int, QByteArray> ret{{Qt::DisplayRole, "display"}, {(int)Roles::Value, "value"}};
  return ret;
}
