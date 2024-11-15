#include "settings.hpp"
#include <QQmlEngine>

pepp::settings::Category::Category(QObject *parent) : QObject(parent) {}

// General
static const char *defaultArchKey = "General/defaultArch";
static const char *defaultAbstractionKey = "General/defaultAbstraction";
static const char *maxRecentFilesKey = "General/maxRecentFiles";
static const char *showMenuHotkeysKey = "General/showMenuHotkeys";
static const char *showChangeDialogKey = "General/showChangeDialog";
// Editor
static const char *visualizeWhitespaceKey = "Editor/visualizeWhitespace";
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

pepp::settings::ThemeCategory::ThemeCategory(QObject *parent) : Category(parent) {
  // TODO: load last selected theme from disk and bind to _palette.
  _palette = new Palette(this);
}

pepp::settings::EditorCategory::EditorCategory(QObject *parent) : Category(parent) {}

void pepp::settings::EditorCategory::sync() { _settings.sync(); }

bool pepp::settings::EditorCategory::visualizeWhitespace() const {
  auto value = _settings.value(visualizeWhitespaceKey);
  if (value.isValid()) return value.toBool();
  else {
    _settings.setValue(visualizeWhitespaceKey, _defaultVisualizeWhitespace);
    return _defaultVisualizeWhitespace;
  }
}

void pepp::settings::EditorCategory::setVisualizeWhitespace(bool visualize) {
  _settings.setValue(visualizeWhitespaceKey, visualize);
  emit visualizeWhitespaceChanged();
}

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

pepp::settings::detail::AppSettingsData *pepp::settings::detail::AppSettingsData::getInstance() {
  static auto data = new AppSettingsData();
  return data;
}

pepp::settings::detail::AppSettingsData::AppSettingsData() {
  // Naked new, but required to live lifetime of program so I don't care about leaks.
  _categories.append(_general = new GeneralCategory(nullptr));
  _categories.append(_theme = new ThemeCategory(nullptr));
  _categories.append(_editor = new EditorCategory(nullptr));
  _categories.append(_simulator = new SimulatorCategory(nullptr));
  _categories.append(_keymap = new KeyMapCategory(nullptr));
}

pepp::settings::AppSettings::AppSettings(QObject *parent)
    : QObject(parent), _data(detail::AppSettingsData::getInstance()) {
  /*void defaultArchChanged();
  void defaultAbstractionChanged();
  void maxRecentFilesChanged();
  void showMenuHotkeysChanged();
  void showChangeDialogChanged();
  QObject::connect(general(), &GeneralCategory::defaultArchChanged, this, &AppSettings::)*/
}

QList<pepp::settings::Category *> pepp::settings::AppSettings::categories() const { return _data->categories(); }
pepp::settings::GeneralCategory *pepp::settings::AppSettings::general() const { return _data->general(); }
pepp::settings::ThemeCategory *pepp::settings::AppSettings::theme() const { return _data->theme(); }
pepp::settings::Palette *pepp::settings::AppSettings::themePalette() const { return _data->themePalette(); }
pepp::settings::EditorCategory *pepp::settings::AppSettings::editor() const { return _data->editor(); }
pepp::settings::SimulatorCategory *pepp::settings::AppSettings::simulator() const { return _data->simulator(); }
pepp::settings::KeyMapCategory *pepp::settings::AppSettings::keymap() const { return _data->keymap(); }

void pepp::settings::AppSettings::loadPalette(const QString &path) {
  QFile jsonFile(path);
  if (!jsonFile.open(QIODevice::ReadOnly)) return;
  QByteArray ba = jsonFile.readAll();
  jsonFile.close();
  QJsonParseError parseError;
  QJsonDocument doc = QJsonDocument::fromJson(ba, &parseError);

  if (parseError.error != QJsonParseError::NoError)
    qWarning() << "Parse error at" << parseError.offset << ":" << parseError.errorString();

  auto pal = themePalette();
  pal->updateFromJson(doc.object());
}

void pepp::settings::AppSettings::sync() {
  for (auto category : categories()) category->sync();
}
