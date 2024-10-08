#pragma once

#include <QObject>
#include <QtQmlIntegration>

class PlatformDetector : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool isWindows READ isWindows CONSTANT)
  Q_PROPERTY(bool isMac READ isMac CONSTANT)
  Q_PROPERTY(bool isLinux READ isLinux CONSTANT)
  Q_PROPERTY(bool isWASM READ isWASM CONSTANT)
  QML_NAMED_ELEMENT(PlatformDetector)
  QML_SINGLETON

public:
  explicit PlatformDetector(QObject *parent = nullptr);
  Q_INVOKABLE bool isWindows() const;
  Q_INVOKABLE bool isMac() const;
  Q_INVOKABLE bool isLinux() const;
  Q_INVOKABLE bool isWASM() const;
};
