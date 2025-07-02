#pragma once

#include <QObject>
#include <QtQmlIntegration>

class FileIO : public QObject {
  Q_OBJECT
  QML_NAMED_ELEMENT(FileIO)

public:
  FileIO(QObject *parent = nullptr);
  Q_INVOKABLE void save(const QString &filename, const QString &data);
  Q_INVOKABLE void load(const QString &filters);
signals:
  Q_INVOKABLE void codeLoaded(const QString &fileName, const QString &fileContent, int arch, int abstraction);
};
