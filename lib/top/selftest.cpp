#include "selftest.hpp"
#include <catch/catch.hpp>

SelfTest::SelfTest(QObject *parent) : QAbstractTableModel(parent) {}

QVariant SelfTest::headerData(int section, Qt::Orientation orientation, int role) const {
  switch (role) {
  case Qt::DisplayRole:
    if (section == 0) return QString("Test Name");
    else if (section == 1) return QString("Enabled");
    else if (section == 2) return QString("Tags");
    else if (section == 3) return QString("Pass/Fail");
    else if (section == 4) return QString("Duration");
    else if (section == 5) return QString("Checks");
  default: break;
  }
  return {};
}

int SelfTest::columnCount(const QModelIndex &) const { return 6; }

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
  case 1: return Qt::CheckState::PartiallyChecked;
  case 2: return QString::fromStdString(tc.getTestCaseInfo().tagsAsString());
  case 3: return Qt::CheckState::PartiallyChecked;
  case 4: return QTime(0, 0, 0).addMSecs(10 * index.row()).toString("mm:ss.zzz");
  case 5: return 0;
  }
  return {};
}
