#pragma once

#include <QObject>
#include <QtQmlIntegration>

class WASMIO : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString loadedName READ loadedName NOTIFY loaded)
  QML_ELEMENT

public:
  WASMIO(QObject *parent = nullptr);
  Q_INVOKABLE void save(const QString &filename, const QString &data);
  Q_INVOKABLE void load(const QString &nameFilter);
  QString loadedName() const;

signals:
  void loaded();

private:
  QString _loadedName;
};
