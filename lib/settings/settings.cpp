#include "settings.hpp"
#include <QQmlEngine>

ApplicationPreferences::ApplicationPreferences(QObject *parent) : QObject{parent} {}

ApplicationPreferences *ApplicationPreferences::create(QQmlEngine *eng, QJSEngine *) {
  return new ApplicationPreferences(eng);
}
