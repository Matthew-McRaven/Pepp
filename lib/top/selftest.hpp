#pragma once

#include <QAbstractListModel>
#include <QtQmlIntegration>
#include <chrono>
#include <memory>
#if defined(PEPP_HAS_QTCONCURRENT) && PEPP_HAS_QTCONCURRENT == 1
#include <QFuture>
#endif

struct TestCase {
  bool enabled;
  QString tags;
  uint64_t failed = 0, total = 0;
  std::chrono::duration<double, std::milli> duration;
};

class SelfTest : public QAbstractTableModel {
  Q_OBJECT
  QML_NAMED_ELEMENT(SelfTestModel)
  Q_PROPERTY(int selectedTests READ selectedTests NOTIFY selectedTestsChanged)
  Q_PROPERTY(int visibleTests READ visibleTests NOTIFY visibleTestsChanged)
  Q_PROPERTY(bool running READ running NOTIFY runningChanged)

public:
  explicit SelfTest(QObject *parent = nullptr);
  ~SelfTest() override;

  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex &index, const QVariant &value, int role) override;
  QHash<int, QByteArray> roleNames() const override;

  inline int selectedTests() const { return _selected; };
  inline int visibleTests() const { return 72; };
  inline bool running() const { return _running; };
  Q_INVOKABLE void runSelectedTests();
  Q_INVOKABLE void runAllTests();
  Q_INVOKABLE void stop();

signals:
  void selectedTestsChanged();
  void visibleTestsChanged();
  void runningChanged();

private:
  void runFiltered(std::function<bool(const TestCase &)> filter);
  bool _running = false;
  int _selected = 0;
  std::map<int, std::unique_ptr<TestCase>> _tests;
#if defined(PEPP_HAS_QTCONCURRENT) && PEPP_HAS_QTCONCURRENT == 1
  QFuture<void> _fut;
#endif
};
