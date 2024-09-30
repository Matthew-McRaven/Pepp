#include "settings.hpp"
#include <QQmlEngine>

AppSettings::AppSettings(QObject *parent) : QObject{parent} {
  _p = new GeneralCategory(this);

  _categories.append(_p);
}

GeneralCategory::GeneralCategory(QObject *parent) : QObject(parent) {}
