#pragma once

#include <QAbstractListModel>
#include <QtQmlIntegration>

class SelfTest : public QAbstractListModel {
  Q_OBJECT
  QML_NAMED_ELEMENT(SelfTestModel)

public:
  explicit SelfTest(QObject *parent = nullptr);

  // Header:
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
};
