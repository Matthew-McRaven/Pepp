#pragma once

#include <QObject>
#include <QtQmlIntegration>

class FileIO : public QObject {
  Q_OBJECT
  QML_NAMED_ELEMENT(FileIO)

public:
  FileIO(QObject *parent = nullptr);
  Q_INVOKABLE void save(const QString &filename, const QString &data);
  Q_INVOKABLE void loadCodeViaDialog(const QString &filters);
#ifndef __EMSCRIPTEN__
  Q_INVOKABLE void loadCodeFromFile(const QString &name, int arch, int abs);

private:
  QByteArray load(const QString &fileName);
#endif

signals:
  Q_INVOKABLE void codeLoaded(const QString &fileName, const QString &fileContent, int arch, int abstraction);
};
