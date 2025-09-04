#pragma once
#include <QObject>
#include <QtGui/qfont.h>
#include <QtQmlIntegration>

class FontUtilsHelper : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("")
public:
  FontUtilsHelper(QFont font, QObject *parent = nullptr);
  Q_INVOKABLE FontUtilsHelper *h1();
  Q_INVOKABLE FontUtilsHelper *h2();
  Q_INVOKABLE FontUtilsHelper *h3();
  Q_INVOKABLE FontUtilsHelper *bold();
  Q_INVOKABLE FontUtilsHelper *italicize();
  Q_INVOKABLE QFont font();

private:
  QFont _font;
};

class FontUtils : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
public:
  FontUtils(QObject *parent = nullptr);
  Q_INVOKABLE FontUtilsHelper *fromFont(QFont font);
};
