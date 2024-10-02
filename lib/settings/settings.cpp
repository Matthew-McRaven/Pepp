#include "settings.hpp"
#include <QQmlEngine>

pepp::settings::Category::Category(QObject *parent) : QObject(parent) {}
pepp::settings::GeneralCategory::GeneralCategory(QObject *parent) : Category(parent) {}
pepp::settings::ThemeCategory::ThemeCategory(QObject *parent) : Category(parent) {}
pepp::settings::EditorCategory::EditorCategory(QObject *parent) : Category(parent) {}
pepp::settings::KeyMapCategory::KeyMapCategory(QObject *parent) : Category(parent) {}

pepp::settings::AppSettings::AppSettings(QObject *parent) : QObject{parent} {
  _categories.append(_general = new GeneralCategory(this));
  _categories.append(_theme = new ThemeCategory(this));
  _categories.append(_editor = new EditorCategory(this));
  _categories.append(_keymap = new KeyMapCategory(this));
}

void pepp::settings::AppSettings::sync() {
  for (auto category : _categories) category->sync();
}
