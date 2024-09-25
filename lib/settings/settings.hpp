#pragma once

#include <QObject>
#include <QtQmlIntegration>

class QJSEngine;
class QQmlEngine;

class ApplicationPreferences : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString general MEMBER _general)
  QML_SINGLETON
  QML_NAMED_ELEMENT(ApplicationPreferences)

public:
  explicit ApplicationPreferences(QObject *parent = nullptr);
  static ApplicationPreferences *create(QQmlEngine *, QJSEngine *);

private:
  QString _general;
};
