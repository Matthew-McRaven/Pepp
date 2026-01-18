#include "selftest.hpp"
#include <catch/catch.hpp>

SelfTest::SelfTest(QObject *parent) : QAbstractTableModel(parent) {}

QVariant SelfTest::headerData(int section, Qt::Orientation orientation, int role) const {
  switch (role) {
  case Qt::DisplayRole:
    if (section == 0) return QString("Test Name");
    else if (section == 1) return QString("Tags");
  default: break;
  }
  return {};
}

int SelfTest::columnCount(const QModelIndex &parent) const { return 2; }

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

  switch (index.column()) {
  case 0: return QString::fromStdString(tc.getTestCaseInfo().name);
  case 1: return QString::fromStdString(tc.getTestCaseInfo().tagsAsString());
  }
  return {};
}
