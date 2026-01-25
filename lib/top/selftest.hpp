#pragma once

#include <QAbstractListModel>
#include <QtQmlIntegration>

// TestCase brings in Catch.hpp
// I DO NOT want that header to propogate elsewhere.
struct TestCase;
class SelfTest : public QAbstractTableModel {
  Q_OBJECT
  QML_NAMED_ELEMENT(SelfTestModel)
  Q_PROPERTY(int selectedTests READ selectedTests NOTIFY selectedTestsChanged)
  Q_PROPERTY(int visibleTests READ visibleTests NOTIFY visibleTestsChanged)

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

  Q_INVOKABLE void runSelectedTests();
  Q_INVOKABLE void runAllTests();
signals:
  void selectedTestsChanged();
  void visibleTestsChanged();

private:
  // Can't use unique ptr, because Qt MOC explodes w/o definition of TestCase.
  std::map<int, TestCase *> _tests;
  int _selected = 0;
};
