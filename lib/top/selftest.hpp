#pragma once

#include <QAbstractListModel>
#include <QRegularExpression>
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
  Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
  Q_PROPERTY(int selectedTests READ selectedTests NOTIFY selectedTestsChanged)
  Q_PROPERTY(bool running READ running NOTIFY runningChanged)
  Q_PROPERTY(QString workingDirectory READ workingDirectory CONSTANT);

public:
  explicit SelfTest(QObject *parent = nullptr);
  ~SelfTest() override;

  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex &index, const QVariant &value, int role) override;
  QHash<int, QByteArray> roleNames() const override;

  inline int progress() const { return _progress; };
  inline int selectedTests() const { return _selected; };
  inline bool running() const { return _running; };
  inline QString workingDirectory() const { return _temp_cwd.path(); }
  Q_INVOKABLE void runSelectedTests();
  Q_INVOKABLE void runAllTests();
  Q_INVOKABLE void stop();
  Q_INVOKABLE void enableAll();
  Q_INVOKABLE void disableAll();

signals:
  void progressChanged();
  void selectedTestsChanged();
  void runningChanged();

private:
  void runFiltered(std::function<bool(const TestCase &)> filter);
  bool _running = false;
  int _selected = 0, _progress = 0;
  std::map<int, std::unique_ptr<TestCase>> _tests;
  QTemporaryDir _temp_cwd;
#if defined(PEPP_HAS_QTCONCURRENT) && PEPP_HAS_QTCONCURRENT == 1
  QFuture<void> _fut;
#endif
};

class SelfTestFilterModel : public QSortFilterProxyModel {
  Q_OBJECT
  Q_PROPERTY(QString regex READ regex WRITE setRegex NOTIFY regexChanged)
  QML_NAMED_ELEMENT(SelfTestFilterModel);

public:
  SelfTestFilterModel(QObject *parent = nullptr);
  QString regex() const;
  void setRegex(QString re);
  Q_INVOKABLE void enableAll();
  Q_INVOKABLE void disableAll();

signals:
  void regexChanged();

protected:
  bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
  QRegularExpression _re;
};
