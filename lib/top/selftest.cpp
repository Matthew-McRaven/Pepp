#include "selftest.hpp"
#include <catch/catch.hpp>

struct TestCase {
  bool enabled;
};

SelfTest::SelfTest(QObject *parent) : QAbstractTableModel(parent) {
  const auto &hub = Catch::getRegistryHub();
  const auto count = hub.getTestCaseRegistry().getAllTests().size();
  for (size_t i = 0; i < count; i++) _tests[i] = new TestCase{false};
}

SelfTest::~SelfTest() {
  for (auto &[k, v] : _tests) delete v;
}

QVariant SelfTest::headerData(int section, Qt::Orientation orientation, int role) const {
  switch (role) {
  case Qt::DisplayRole:
    if (section == 0) return QString("Test Name");
    else if (section == 1) return QString("Enabled");
    else if (section == 2) return QString("Tags");
    else if (section == 3) return QString("Duration");
    else if (section == 4) return QString("Fails");
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
  return _tests.size();
}

QVariant SelfTest::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) return QVariant();
  const auto &hub = Catch::getRegistryHub();
  auto tc = hub.getTestCaseRegistry().getAllTests().at(index.row());

  switch (role) {
  case Qt::DisplayRole:
    switch (index.column()) {
    case 0: return QString::fromStdString(tc.getTestCaseInfo().name);
    case 1: return _tests.at(index.row())->enabled ? Qt::CheckState::Checked : Qt::CheckState::Unchecked;
    case 2: return QString::fromStdString(tc.getTestCaseInfo().tagsAsString());
    case 3: return QTime(0, 0, 0).addMSecs(10 * index.row()).toString("mm:ss.zzz");
    case 4: return QString("0 Fails");
    case 5: return QString("100 total");
    }
  case Qt::UserRole + 1:
    if (index.column() == 1) return "check";
    else return "text";
  }
  return {};
}

bool SelfTest::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (!index.isValid()) return false;
  if (index.column() == 1 && role == Qt::DisplayRole) {
    auto as_bool = value.toBool();
    if (as_bool) _selected++;
    else _selected--;
    _tests[index.row()]->enabled = as_bool;
    emit selectedTestsChanged();
    emit dataChanged(index, index, {role});
    return true;
  }
  return false;
}

QHash<int, QByteArray> SelfTest::roleNames() const {
  static const QHash<int, QByteArray> roles = {
      {Qt::DisplayRole, "display"},
      {Qt::UserRole + 1, "type"},
  };
  return roles;
}

void SelfTest::runSelectedTests() {}

void SelfTest::runAllTests() {}
