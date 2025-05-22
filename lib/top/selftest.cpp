#include "selftest.hpp"
#include <catch.hpp>

SelfTest::SelfTest(QObject *parent) : QAbstractListModel(parent) {}

QVariant SelfTest::headerData(int section, Qt::Orientation orientation, int role) const {
  // FIXME: Implement me!
  return {};
}

int SelfTest::rowCount(const QModelIndex &parent) const {
  // For list models only the root node (an invalid parent) should return the list's size. For all
  // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
  if (parent.isValid()) return 0;
  const auto &hub = Catch::getRegistryHub();
  return hub.getTestCaseRegistry().getAllTests().size();
}

QVariant SelfTest::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) return QVariant();
  const auto &hub = Catch::getRegistryHub();
  auto tc = hub.getTestCaseRegistry().getAllTests().at(index.row());
  return QString::fromStdString(tc.getTestCaseInfo().name);
}
