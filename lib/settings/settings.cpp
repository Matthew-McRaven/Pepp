#include "settings.hpp"
#include <QQmlEngine>

pepp::settings::Category::Category(QObject *parent) : QObject(parent) {}

// General
static const char *defaultArchKey = "General/defaultArch";
static const char *defaultAbstractionKey = "General/defaultAbstraction";
static const char *maxRecentFilesKey = "General/maxRecentFiles";
static const char *showMenuHotkeysKey = "General/showMenuHotkeys";
static const char *showChangeDialogKey = "General/showChangeDialog";
// Simulator
static const char *maxStepbackBufferKBKey = "Simulator/maxStepbackBufferKB";

pepp::settings::GeneralCategory::GeneralCategory(QObject *parent) : Category(parent) {}

void pepp::settings::GeneralCategory::sync() { _settings.sync(); }

builtins::Architecture pepp::settings::GeneralCategory::defaultArch() const {
  bool casted = false;
  auto archEnum = QMetaEnum::fromType<builtins::Architecture>();
  auto value = _settings.value(defaultArchKey);
  if (auto asInt = value.toInt(&casted); value.isValid() && casted && archEnum.valueToKey(asInt) != nullptr)
    return static_cast<builtins::Architecture>(asInt);
  else {
    _settings.setValue(defaultArchKey, (int)defaultDefaultArch);
    return defaultDefaultArch;
  }
}

void pepp::settings::GeneralCategory::setDefaultArch(builtins::Architecture arch) {
  _settings.setValue(defaultArchKey, (int)arch);
  emit defaultArchChanged();
}

builtins::Abstraction pepp::settings::GeneralCategory::defaultAbstraction() const {
  bool casted = false;
  auto absEnum = QMetaEnum::fromType<builtins::Abstraction>();
  auto value = _settings.value(defaultAbstractionKey);
  if (auto asInt = value.toInt(&casted); value.isValid() && casted && absEnum.valueToKey(asInt) != nullptr)
    return static_cast<builtins::Abstraction>(asInt);
  else {
    _settings.setValue(defaultAbstractionKey, (int)defaultDefaultAbstraction);
    return defaultDefaultAbstraction;
  }
}

void pepp::settings::GeneralCategory::setDefaultAbstraction(builtins::Abstraction abstraction) {
  _settings.setValue(defaultAbstractionKey, (int)abstraction);
  emit defaultAbstractionChanged();
}

int pepp::settings::GeneralCategory::maxRecentFiles() const {
  bool casted = false;
  auto value = _settings.value(maxRecentFilesKey);
  if (auto asInt = value.toInt(&casted); value.isValid() && casted) return asInt;
  else {
    _settings.setValue(maxRecentFilesKey, defaultMaxRecentFiles);
    return defaultMaxRecentFiles;
  }
}

void pepp::settings::GeneralCategory::setMaxRecentFiles(int max) {
  if (!validateMaxRecentFiles(max)) return;
  _settings.setValue(maxRecentFilesKey, max);
  emit maxRecentFilesChanged();
}

bool pepp::settings::GeneralCategory::showMenuHotkeys() const {
  auto value = _settings.value(showMenuHotkeysKey);
  if (value.isValid()) return value.toBool();
  else {
    _settings.setValue(showMenuHotkeysKey, defaultShowMenuHotkeys);
    return defaultShowMenuHotkeys;
  }
}

void pepp::settings::GeneralCategory::setShowMenuHotkeys(bool show) {
  _settings.setValue(showMenuHotkeysKey, show);
  emit showMenuHotkeysChanged();
}

bool pepp::settings::GeneralCategory::showChangeDialog() const {
  auto value = _settings.value(showChangeDialogKey);
  if (value.isValid()) return value.toBool();
  else {
    _settings.setValue(showChangeDialogKey, defaultShowChangeDialog);
    return defaultShowChangeDialog;
  }
}

void pepp::settings::GeneralCategory::setShowChangeDialog(bool show) {
  _settings.setValue(showChangeDialogKey, show);
  emit showChangeDialogChanged();
}

bool pepp::settings::GeneralCategory::validateMaxRecentFiles(int max) const {
  if (max <= 0 || max > 10) return false;
  return true;
}

pepp::settings::ThemeCategory::ThemeCategory(QObject *parent) : Category(parent) {}
pepp::settings::EditorCategory::EditorCategory(QObject *parent) : Category(parent) {}
pepp::settings::SimulatorCategory::SimulatorCategory(QObject *parent) {}

void pepp::settings::SimulatorCategory::sync() { _settings.sync(); }

int pepp::settings::SimulatorCategory::minMaxStepbackBufferKB() const { return 10; }

int pepp::settings::SimulatorCategory::maxMaxStepbackBufferKB() const { return 200'000; }

int pepp::settings::SimulatorCategory::maxStepbackBufferKB() const {
  bool casted = false;
  auto value = _settings.value(maxStepbackBufferKBKey);
  if (auto asInt = value.toInt(&casted); value.isValid() && casted) return asInt;
  else {
    _settings.setValue(maxStepbackBufferKBKey, _defaultMaxStepbackBufferKB);
    return _defaultMaxStepbackBufferKB;
  }
}

void pepp::settings::SimulatorCategory::setMaxStepbackBufferKB(int max) {
  if (!validateMaxStepbackBufferKB(max)) return;
  _settings.setValue(maxStepbackBufferKBKey, max);
  emit maxStepbackBufferKBChanged();
}

bool pepp::settings::SimulatorCategory::validateMaxStepbackBufferKB(int max) const {
  return max >= minMaxStepbackBufferKB() && max <= maxMaxStepbackBufferKB();
}

pepp::settings::KeyMapCategory::KeyMapCategory(QObject *parent) : Category(parent) {}

pepp::settings::AppSettings::AppSettings(QObject *parent) : QObject{parent} {
  _categories.append(_general = new GeneralCategory(this));
  _categories.append(_theme = new ThemeCategory(this));
  _categories.append(_editor = new EditorCategory(this));
  _categories.append(_simulator = new SimulatorCategory(this));
  _categories.append(_keymap = new KeyMapCategory(this));
}

void pepp::settings::AppSettings::sync() {
  for (auto category : _categories) category->sync();
}
