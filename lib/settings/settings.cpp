#include "settings.hpp"
#include <QQmlEngine>

pepp::settings::Category::Category(QObject *parent) : QObject(parent) {}

// General
static const char *defaultArchKey = "General/defaultArch";
static const char *defaultAbstractionKey = "General/defaultAbstraction";
static const char *showDebugKey = "General/showDebug";
static const char *maxRecentFilesKey = "General/maxRecentFiles";
static const char *showMenuHotkeysKey = "General/showMenuHotkeys";
static const char *showChangeDialogKey = "General/showChangeDialog";
static const char *allowExternFigs = "General/allowExternalFigures";
static const char *externFigDir = "General/externalFigureDirectory";
// Editor
static const char *visualizeWhitespaceKey = "Editor/visualizeWhitespace";
// Palette
static const char *themeRootKey = "Theme";
static const char *themePathKey = "Theme/activePath";
// Simulator
static const char *maxStepbackBufferKBKey = "Simulator/maxStepbackBufferKB";

pepp::settings::GeneralCategory::GeneralCategory(QObject *parent) : Category(parent) {}

void pepp::settings::GeneralCategory::sync() { _settings.sync(); }

void pepp::settings::GeneralCategory::resetToDefault() {
  setDefaultArch(defaultDefaultArch);
  setDefaultAbstraction(defaultDefaultAbstraction);
  setMaxRecentFiles(defaultMaxRecentFiles);
  setShowMenuHotkeys(defaultShowMenuHotkeys);
  setShowChangeDialog(defaultShowChangeDialog);
  setAllowExternalFigures(defaultAllowExternalFigures);
}

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

bool pepp::settings::GeneralCategory::showDebugComponents() const {
  auto value = _settings.value(showDebugKey);
  if (value.isValid()) return value.toBool();
  else {
    _settings.setValue(showDebugKey, defaultShowDebugComponents);
    return defaultShowDebugComponents;
  }
}

void pepp::settings::GeneralCategory::setShowDebugComponents(bool show) {
  _settings.setValue(showDebugKey, show);
  emit showDebugComponentsChanged();
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

bool pepp::settings::GeneralCategory::allowExternalFigures() const {
  auto value = _settings.value(allowExternFigs);
  if (value.isValid()) return value.toBool();
  else {
    _settings.setValue(allowExternFigs, defaultAllowExternalFigures);
    return defaultAllowExternalFigures;
  }
}
void pepp::settings::GeneralCategory::setAllowExternalFigures(bool allow) {
  _settings.setValue(allowExternFigs, allow);
  emit allowExternalFiguresChanged();
}

QString pepp::settings::GeneralCategory::externalFigureDirectory() const {
  auto value = _settings.value(externFigDir);
  if (value.isValid()) return value.toString();
  else {
    _settings.setValue(externFigDir, "");
    return "";
  }
}

void pepp::settings::GeneralCategory::setExternalFigureDirectory(const QString &path) {
  _settings.setValue(externFigDir, path);
  emit externalFigureDirectoryChanged();
}

bool pepp::settings::GeneralCategory::validateMaxRecentFiles(int max) const {
  if (max <= 0 || max > 10) return false;
  return true;
}
static const auto defaultPath = ":/themes/Default.theme";
pepp::settings::ThemeCategory::ThemeCategory(QObject *parent) : Category(parent) {
  auto pal = new Palette(this);
  // In WASM, the IDBFS filesystem mounted at / will not be ready when we try to load the theme,
  // So, we persist the theme into the Qt settings FS as a form of cache.
  // By the time the settings pane is opened, the IDBFS should be synced.
#if defined(Q_OS_WASM)
  _settings.beginGroup(themeRootKey);
  pal->updateFromSettings(_settings);
  _settings.endGroup();
#else
  if (auto path = _settings.value(themePathKey); path.isValid()) {
    // Attempt to load from path. If it fails, set to default and load that.
    if (!loadFromPath(pal, path.toString())) {
      _settings.setValue(themePathKey, defaultPath);
      loadFromPath(pal, defaultPath);
    }
  }
#endif

  for (auto item : pal->items())
    connect(item, &PaletteItem::preferenceChanged, this, &ThemeCategory::onPaletteItemChanged);
  _palette = pal;
}
void pepp::settings::ThemeCategory::onPaletteItemChanged() {
  auto sender = QObject::sender();
#ifdef Q_OS_WASM
  if (auto item = qobject_cast<PaletteItem *>(sender); item && true) {
    QSettings nested;
    nested.beginGroup(themeRootKey);
    nested.beginGroup(PaletteRoleHelper::string(item->ownRole()));
    item->toSettings(nested);
    nested.endGroup();
    nested.endGroup();
  }
#endif
}

QString pepp::settings::ThemeCategory::themePath() const {
  auto value = _settings.value(themePathKey);
  if (value.isValid()) return value.toString();
  else {
    _settings.setValue(themePathKey, defaultPath);
    return defaultPath;
  }
}

void pepp::settings::ThemeCategory::setThemePath(const QString &path) {
  _settings.setValue(themePathKey, path);
  emit themePathChanged();
}

void pepp::settings::ThemeCategory::sync() {
// Only sync values on WASM to avoid needlessly polluting registry / plist / etc.
#ifdef Q_OS_WASM
  // Write palette toDisk.
  _settings.beginGroup(themeRootKey);
  _palette->toSettings(_settings);
  _settings.endGroup();
#endif
  // Synchronize other settings.
  _settings.sync();
}

void pepp::settings::ThemeCategory::resetToDefault() {
  // Remove all child keys, including PaletteItems
  _settings.remove(themeRootKey);
  setThemePath(defaultPath);
  loadFromPath(_palette, defaultPath);
}

bool pepp::settings::ThemeCategory::loadFromPath(Palette *pal, const QString &path) {
  QFile jsonFile(path);
  if (!jsonFile.open(QIODevice::ReadOnly)) return false;
  QByteArray ba = jsonFile.readAll();
  jsonFile.close();
  QJsonParseError parseError;
  QJsonDocument doc = QJsonDocument::fromJson(ba, &parseError);

  if (parseError.error != QJsonParseError::NoError)
    qWarning() << "Parse error at" << parseError.offset << ":" << parseError.errorString();

  auto ret = pal->updateFromJson(doc.object());
// Only sync values on WASM to avoid needlessly polluting registry / plist / etc.
#ifdef Q_OS_WASM
  if (ret) {
    _settings.beginGroup(themeRootKey);
    pal->toSettings(_settings);
    _settings.endGroup();
  }
#endif
  return ret;
}

pepp::settings::EditorCategory::EditorCategory(QObject *parent) : Category(parent) {}

void pepp::settings::EditorCategory::sync() { _settings.sync(); }

void pepp::settings::EditorCategory::resetToDefault() { setVisualizeWhitespace(_defaultVisualizeWhitespace); }

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

void pepp::settings::SimulatorCategory::resetToDefault() { setMaxStepbackBufferKB(_defaultMaxStepbackBufferKB); }

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
  /*_categories.append(*/ _keymap = new KeyMapCategory(nullptr); /*);*/
}

pepp::settings::AppSettings::AppSettings(QObject *parent)
    : QObject(parent), _data(detail::AppSettingsData::getInstance()) {}

QList<pepp::settings::Category *> pepp::settings::AppSettings::categories() const { return _data->categories(); }
pepp::settings::GeneralCategory *pepp::settings::AppSettings::general() const { return _data->general(); }
pepp::settings::ThemeCategory *pepp::settings::AppSettings::theme() const { return _data->theme(); }
pepp::settings::Palette *pepp::settings::AppSettings::themePalette() const { return _data->themePalette(); }
pepp::settings::EditorCategory *pepp::settings::AppSettings::editor() const { return _data->editor(); }
pepp::settings::SimulatorCategory *pepp::settings::AppSettings::simulator() const { return _data->simulator(); }
pepp::settings::KeyMapCategory *pepp::settings::AppSettings::keymap() const { return _data->keymap(); }

void pepp::settings::AppSettings::loadPalette(const QString &path) {
  auto pal = themePalette();
  if (_data->theme()->loadFromPath(pal, path)) _data->theme()->setThemePath(path);
}

void pepp::settings::AppSettings::resetToDefault() {
  for (auto category : categories()) category->resetToDefault();
}

void pepp::settings::AppSettings::sync() {
  for (auto category : categories()) category->sync();
}
