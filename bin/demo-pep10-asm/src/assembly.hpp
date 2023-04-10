#pragma once
#include "builtins/figure.hpp"
#include <QObject>
class AssemblyManger : public QObject {
  Q_OBJECT
  builtins::Figure *_active = nullptr;
  Q_PROPERTY(QString usrTxt READ usrTxt NOTIFY usrTxtChanged);
  Q_PROPERTY(QString osTxt READ osTxt NOTIFY osTxtChanged);
  QString _usrTxt = "", _osTxt = "";

public:
  AssemblyManger() = default;
  QString usrTxt() { return _usrTxt; }
  QString osTxt() { return _osTxt; }
public slots:
  void onSelectionChanged(builtins::Figure *figure);
  void onAssemble();
  void clearUsrTxt();
  void clearOsTxt();
signals:
  void osTxtChanged();
  void usrTxtChanged();
};
