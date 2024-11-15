#pragma once

#include <QObject>
#include <QtQmlIntegration>

class FileIO : public QObject {
  Q_OBJECT
  QML_NAMED_ELEMENT(FileIO)
  QML_SINGLETON

public:
  FileIO(QObject *parent = nullptr);
  Q_INVOKABLE void save(const QString &filename, const QString &data);
};
