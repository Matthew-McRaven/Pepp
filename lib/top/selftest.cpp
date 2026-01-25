#include "selftest.hpp"
#include <catch/catch.hpp>
#if defined(PEPP_HAS_QTCONCURRENT) && PEPP_HAS_QTCONCURRENT == 1
#include <QFuture>
#include <QPromise>
#include <QtConcurrentRun>
#include <chrono>
#endif

// Minimal IStream that discards all output
struct NullStream final : Catch::IStream {
  std::ostringstream sink;
  std::ostream &stream() override { return sink; }
  bool isConsole() const override { return false; }
};

// Helper to build a reporter (or listener chain) from the registry.
static Catch::IEventListenerPtr makeReporter(Catch::IConfig const &cfg) {
  Catch::Detail::unique_ptr<Catch::IStream> ns{new NullStream};

  Catch::ReporterConfig rc{&cfg, std::move(ns), Catch::ColourMode::None, std::map<std::string, std::string>{}};

  // Pick any built-in reporter name you like ("console", "compact", "junit", ...).
  // This returns IEventListenerPtr.
  return Catch::getRegistryHub().getReporterRegistry().create("compact", std::move(rc));
}

struct TestCase {
  bool enabled;
  QStringList tags;
  uint64_t failed = 0, total = 0;
  std::chrono::duration<double, std::milli> duration;
};

SelfTest::SelfTest(QObject *parent) : QAbstractTableModel(parent) {
  const auto &hub = Catch::getRegistryHub();
  const auto count = hub.getTestCaseRegistry().getAllTests().size();
  for (size_t i = 0; i < count; i++) {
    auto tcase = hub.getTestCaseRegistry().getAllTests().at(i);
    QStringList tags;
    for (const auto &tag : tcase.getTestCaseInfo().tags)
      tags.append(QString::fromStdString({tag.original.data(), tag.original.size()}));
    _tests[i] = new TestCase{.enabled = false, .tags = tags};
  }
}

SelfTest::~SelfTest() {
  for (auto &[k, v] : _tests) delete v;
}

QVariant SelfTest::headerData(int section, Qt::Orientation, int role) const {
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
  auto t = _tests.at(index.row());
  switch (role) {
  case Qt::DisplayRole:
    switch (index.column()) {
    case 0: return QString::fromStdString(tc.getTestCaseInfo().name);
    case 1: return _tests.at(index.row())->enabled ? Qt::CheckState::Checked : Qt::CheckState::Unchecked;
    case 2: return _tests.at(index.row())->tags.join(", ");
    case 3: return t->duration.count() < 0.001 ? QString("n/a") : QString("%1 ms").arg(t->duration.count(), 0, 'f', 2);
    case 4: return QString("%1 failed").arg(t->failed);
    case 5: return QString("%1 total").arg(t->total);
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

void SelfTest::runSelectedTests() {
  runFiltered([](const TestCase &tc) { return tc.enabled; });
}

void SelfTest::runAllTests() {
  runFiltered([](const TestCase &) { return true; });
}

void SelfTest::runFiltered(std::function<bool(const TestCase &)> filter) {
  if (_running) return;
  _running = true;
  emit runningChanged();
#if defined(PEPP_HAS_QTCONCURRENT) && PEPP_HAS_QTCONCURRENT == 1
  QFuture<void> future = QtConcurrent::run([filter, this](QPromise<void> &promise) {
    promise.setProgressRange(0, _tests.size());
#endif
    const auto &hub = Catch::getRegistryHub();
    Catch::ConfigData cd;
    cd.testsOrTags.clear();
    cd.showSuccessfulTests = false;
    cd.shouldDebugBreak = false;
    cd.defaultColourMode = Catch::ColourMode::None;
    static Catch::Config cfg{cd};
    Catch::getCurrentMutableContext().setConfig(&cfg);
    auto reporter = makeReporter(cfg);
    Catch::RunContext ctx(&cfg, std::move(reporter));
    for (size_t it = 0; it < _tests.size(); it++) {
      auto tc = _tests.at(it);
      if (filter(*tc)) {
        auto t = hub.getTestCaseRegistry().getAllTests().at(it);
        auto t0 = std::chrono::steady_clock::now();
        auto result = ctx.runTest(t);
        auto t1 = std::chrono::steady_clock::now();
        tc->duration = t1 - t0;
        tc->total = result.assertions.total();
        tc->failed = result.assertions.failed;
        emit this->dataChanged(this->index(it, 0), this->index(it, 5), {Qt::DisplayRole});
      }
#if defined(PEPP_HAS_QTCONCURRENT) && PEPP_HAS_QTCONCURRENT == 1
      promise.setProgressValue(it);
      promise.suspendIfRequested();
      if (promise.isCanceled()) return;
#endif
    }
#if defined(PEPP_HAS_QTCONCURRENT) && PEPP_HAS_QTCONCURRENT == 1
  });
  future.then([this]() {
    this->_running = false;
    emit this->runningChanged();
  });
#else
  _running = false;
  emit runningChanged();
#endif
}
