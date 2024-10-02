#include "settings.hpp"
#include <QQmlEngine>

pepp::settings::Category::Category(QObject *parent) : QObject(parent) {}
pepp::settings::GeneralCategory::GeneralCategory(QObject *parent) : Category(parent) {}
pepp::settings::ThemeCategory::ThemeCategory(QObject *parent) : Category(parent) {}

pepp::settings::AppSettings::AppSettings(QObject *parent) : QObject{parent} {
  _categories.append(_general = new GeneralCategory(this));
  _categories.append(_theme = new ThemeCategory(this));
}
