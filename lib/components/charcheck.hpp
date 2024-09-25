#pragma once

#include <QFont>
#include <QObject>
#include <QtQmlIntegration>

class CharCheck : public QObject {
  Q_OBJECT
  QML_ELEMENT

public:
  explicit CharCheck(QObject *parent = nullptr);
  Q_INVOKABLE bool isCharSupported(const QString &character, const QFont &font);
  Q_INVOKABLE QFont noMerge(const QFont &font);
};
