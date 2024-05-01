#pragma once

#include <QObject>

class CharCheck : public QObject {
  Q_OBJECT
public:
  explicit CharCheck(QObject *parent = nullptr);
  Q_INVOKABLE bool isCharSupported(const QString &character, const QFont &font);
  Q_INVOKABLE QFont noMerge(const QFont &font);
};
