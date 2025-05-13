#pragma once
#include <QObject>
#include <QtQmlIntegration>

class ObjectUtilities : public QObject {
  Q_OBJECT
  Q_PROPERTY(int bytesPerRow READ bytesPerRow WRITE setBytesPerRow NOTIFY bytesPerRowChanged)
  QML_NAMED_ELEMENT(ObjectUtilities)

public:
  explicit ObjectUtilities(QObject *parent = nullptr);
  Q_INVOKABLE static bool valid(int key);
  Q_INVOKABLE QString format(QString input, bool includeZZ = true) const;
  // Remove spaces and non space/hex chars
  Q_INVOKABLE QString normalize(QString input) const;
public slots:
  void setBytesPerRow(int bytes);
  int bytesPerRow() const;
signals:
  void bytesPerRowChanged();

private:
  int _bytesPerRow = 16;
};
