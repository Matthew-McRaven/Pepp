#include "selftest.hpp"

SelfTest::SelfTest(QObject *parent) : QAbstractListModel(parent) {}

QVariant SelfTest::headerData(int section, Qt::Orientation orientation, int role) const {
  // FIXME: Implement me!
  return {};
}

int SelfTest::rowCount(const QModelIndex &parent) const {
  // For list models only the root node (an invalid parent) should return the list's size. For all
  // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
  if (parent.isValid()) return 0;

  // FIXME: Implement me!
  return 10;
}

QVariant SelfTest::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) return QVariant();

  // FIXME: Implement me!
  return "Hello World!";
}
