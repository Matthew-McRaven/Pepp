#pragma once

#include <QAbstractListModel>
#include <QtQmlIntegration>

class SelfTest : public QAbstractTableModel {
  Q_OBJECT
  QML_NAMED_ELEMENT(SelfTestModel)
  Q_PROPERTY(int selectedTests READ selectedTests NOTIFY selectedTestsChanged)
  Q_PROPERTY(int visibleTests READ visibleTests NOTIFY visibleTestsChanged)

public:
  explicit SelfTest(QObject *parent = nullptr);

  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

  inline int selectedTests() const { return 32; };
  inline int visibleTests() const { return 72; };
signals:
  void selectedTestsChanged();
  void visibleTestsChanged();

private:
};
