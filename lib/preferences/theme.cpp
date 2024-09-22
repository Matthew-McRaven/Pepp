#include "theme.hpp"

#include <QCoreApplication> //  For application executable path
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaEnum> //  Cast enum integer to character name
#include <QSettings> //  Persist user state from last session
#include <QStandardPaths>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

//  Path for saving in registry or ini file
const char *Theme::themeSettings = "ThemeSettings";

Theme::Theme(QObject *parent) : QObject{parent}, font_("Courier Prime", 12) {
  prefs_.reserve(Themes::Roles::Total);

  //  Read system themes from QRC file
  systemPath_ = ":/themes/";

//  Location for user themes
#ifdef __EMSCRIPTEN__
  userPath_ = "/themes/";
#else
  userPath_ = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/";
#endif

  //  Load system and user themes
  loadThemeList();

  //  See if themes were found
  if (themes_.size() > 0) {

#ifdef __EMSCRIPTEN__
    QSettings settings(QSettings::Format::WebLocalStorageFormat, QSettings::SystemScope,
                       QCoreApplication::organizationName(), QCoreApplication::applicationName());
#else
    QSettings settings;
#endif
    settings.beginGroup(themeSettings);

    //  Retrieve last theme theme
    auto lastTheme = settings.value("theme").toString();

    if (lastTheme.isEmpty()) {
      auto def = systemPath_ + "Default.theme";
      //  Fallback to default if no previous theme
      load(systemPath_ + "Default.theme");
      settings.setValue("theme", def);
    } else {
      load(lastTheme);
    }

    //  Load font
    const auto family = settings.value("fontFamily").toString();
    const auto size = settings.value("fontSize").toInt();

    //  If values are in registery, use them. Otherwise keep default
    if (!family.isEmpty() && size > 0) setFont(family, size);
    else {
      settings.setValue("fontFamily", font_.family());
      settings.setValue("fontSize", font_.pointSize());
    }

    //  Load last theme
    settings.endGroup();
    settings.sync();
  } else
    //  System failure. Load hard-coded theme
    load(systemPath_ + "Default.theme");

  //  Used to generate sample file
  // save("Default.theme");
}

Theme::~Theme() {
  //  Save last user settings
#ifdef __EMSCRIPTEN__
  QSettings settings(QSettings::Format::WebLocalStorageFormat, QSettings::SystemScope,
                     QCoreApplication::organizationName(), QCoreApplication::applicationName());
#else
  QSettings settings;
#endif
  settings.beginGroup(themeSettings);

  //  Save current theme
  settings.setValue("theme", currentTheme_);

  //  Save current font
  settings.setValue("fontFamily", font_.family());
  settings.setValue("fontSize", font_.pointSize());

  settings.endGroup();
  settings.sync();
}

void Theme::loadThemeList() {
  QDir dir;

  //  Themes shipped with application
  dir.setPath(systemPath_);
  QFileInfoList list = dir.entryInfoList(QDir::Files);
  for (auto &file : list) {
    if (file.fileName().endsWith(".theme")) themes_.append(file.baseName());
  }

  //  User themes-can be updated
  dir.setPath(userPath_);
  list = dir.entryInfoList(QDir::Files);
  for (auto &file : list) {
    if (file.fileName().endsWith(".theme")) themes_.append(file.baseName());
  }
}

void Theme::load(const QString &file) {
  //  Save current theme path for updates
  currentTheme_ = file;

  QFile jsonFile(file);

  QJsonParseError parseError;

  if (!jsonFile.open(QIODevice::ReadOnly)) {
    //  File could not be opened, load default if empty
    if (prefs_.size() == 0) loadMissing();

    return;
  }

  //  Read contents and close file
  QByteArray ba = jsonFile.readAll();
  jsonFile.close();

  QJsonDocument doc = QJsonDocument::fromJson(ba, &parseError);

  if (parseError.error != QJsonParseError::NoError) {
    qWarning() << "Parse error at" << parseError.offset << ":" << parseError.errorString();
  }

  //  Read data from Json file
  fromJson(doc.object());
  loadMissing();

  // If WASM, must force sync because we might not sync at the end of app runtime.
#ifdef __EMSCRIPTEN__
  QSettings settings(QSettings::Format::WebLocalStorageFormat, QSettings::SystemScope,
                     QCoreApplication::organizationName(), QCoreApplication::applicationName());
  settings.beginGroup(themeSettings);
  //  Save current theme
  settings.setValue("theme", currentTheme_);
  //  Save current font
  settings.setValue("fontFamily", font_.family());
  settings.setValue("fontSize", font_.pointSize());
  settings.endGroup();
  settings.sync();
#endif
  //  Reset save flag
  isDirty_ = false;
}

QJsonObject Theme::toJson() const {
  QJsonObject doc;

  //  Save font structure to document
  doc["name"] = name_;
  doc["version"] = version_;
  doc["system"] = system_;

  //  Font data saved but not used. Fonts are currently set for all
  //  themes. If we decide to have a font for each theme, then
  //  fromJson will be updated to use this data.
  QJsonObject fontData;
  fontData["familyName"] = font_.family();
  fontData["points"] = font_.pointSize();

  //  Save font structure to document
  doc["font"] = fontData;

  QJsonArray prefData;

  //  Save individual preferences to an array
  for (const auto &p : prefs_) {
    prefData.append(p->toJson());
  }

  //  Append preferences to document
  doc["preferences"] = prefData;

  return doc;
}

QString Theme::themePath() const { return currentTheme_; }

void Theme::fromJson(const QJsonObject &json) {
  if (const QJsonValue v = json["name"]; v.isString()) name_ = v.toString();
  if (const QJsonValue v = json["version"]; v.isString())
    // assert( version_ == v.toString());
    version_ = v.toString();
  if (const QJsonValue v = json["system"]; v.isBool()) system_ = v.toBool();
  //  Font data saved but not used. Fonts are currently set for all
  //  themes. If we decide to have a font for each theme, then
  //  fromJson will be updated to use this data.
  /*  if (const QJsonValue v = json["familyName"]; v.isString())
      font_.setFamily(v.toString());
    if (const QJsonValue v = json["points"]; v.isDouble())
       font_.setPointSize(v.toInt());
  */

  //  Remove previous preferences
  prefs_.clear();

  if (const QJsonValue v = json["preferences"]; v.isArray()) {
    const QJsonArray prefsObj = v.toArray();

    //  Set size of preferences equal to items in file
    prefs_.resize(std::max(prefsObj.size(), (qsizetype)Themes::Roles::Total));

    //  Loop through preferences
    for (const QJsonValue &prefObj : prefsObj) {
      //  Create new preference. Pass parent to preference
      //  so that parent manages memory
      Preference *pref = new Preference(this);

      //  Initialize with font from above
      pref->setFont(&font_);

      //  Populate from json object, skip if error parsing
      if (Preference::fromJson(prefObj.toObject(), *pref)) {
        Q_ASSERT(pref->id() < Themes::Roles::Total);

        //  If there is a parsing error, the role will be invalid
        if (pref->id() != Themes::Roles::Invalid)
          //  Preference Id's are based on sequential Role Enum
          prefs_[pref->id()] = pref;
      }
    }
  }
}

//  Callback from QML
void Theme::selectTheme(const QString newTheme) {
  //  If same theme, no changes required
  if (newTheme == name_) return;

  for (const auto &theme : themes_) {
    if (newTheme == theme) {
      QString systemFile = systemPath_ + theme + ".theme";
      QString userFile = userPath_ + theme + ".theme";
      //  Search system first
      if (QFile::exists(systemFile)) {
        load(systemFile);
        break;
      } else if (QFile::exists(userFile)) {
        load(userFile);
        break;
      }
    }
  }
  //  Notify QML that theme has changed
  emit fontChanged();
  emit themesChanged();
  emit preferenceChanged(); //  Required for screen refresh
}

void Theme::exportTheme(const QString file) const {
  auto cleanFile = file;

  //  Remove "file:///" from URL file name
  cleanFile = cleanFile.remove(0, 8);

  save(cleanFile);
}

void Theme::importTheme(const QString file) {
  auto cleanFile = file;

  //  Ensure export system theme is not treated as system
  //  theme on later import
  if (system_) {
    system_ = false;
    cleanFile.replace(".theme", "-Copy.theme");
  }

  //  Remove "file:///" from URL file name
  cleanFile = cleanFile.remove(0, 8);

  load(cleanFile);

  //  Notify QML that theme has changed
  emit fontChanged();
  emit themesChanged();
  emit preferenceChanged();
}

void Theme::copyTheme(const QString theme) {
  //  Store in user data area
  QString fullName = userPath_ + theme + ".theme";

  //  Update theme name to match user update
  name_ = theme;

  //  Ensure not treated as system theme
  system_ = false;

  save(fullName);

  //  Update listview and notify QML
  themes_.append(theme);
  emit themesChanged();
}

void Theme::deleteTheme(const QString theme) {
  //  Store in user data area
  QString fullName = userPath_ + theme + ".theme";

  QFile themeFile(fullName);

  QJsonParseError parseError;

  if (!themeFile.exists()) {
    //  Theme file cannot be found. Just return
    return;
  }

  //  Delete old theme file and entry
  themeFile.remove();
  themes_.removeAt(themes_.indexOf(theme));

  //  Load Default theme
  const QString systemFile = systemPath_ + "Default.theme";
  //  Search system first
  if (QFile::exists(systemFile)) {
    load(systemFile);
  } else
    //  No theme files. Create default theme
    loadMissing();

  //  Let the world know that there is a new theme
  emit fontChanged();
  emit preferenceChanged();
  emit themesChanged();
}

bool Theme::isDirty() const {
  //  Only non-system themes can change or be saved
  return !system_ && isDirty_;
}

//  Used by slots
void Theme::clearIsDirty() { setDirty(false); }

void Theme::setIsDirty() { setDirty(true); }

QString Theme::jsonString() const {
  auto json = toJson();
  return QJsonDocument(json).toJson();
}

void Theme::setDirty(bool flag) {
  //  Only non-system themes can change or be saved
  if (isDirty_ != flag) {
    isDirty_ = flag;
    emit preferenceChanged();
    emit themesChanged();
  }
}

void Theme::saveTheme() {
  //  Only save if theme in non-system and has changed
  if (isDirty() && !currentTheme_.isEmpty()) {
    save(currentTheme_);

    //  Set dirty data flag
    isDirty_ = false;

    //  Signal controls to redraw
    emit preferenceChanged();
    emit themesChanged();
  }
}

bool Theme::save(const QString &file) const {
  QFile saveFile(file);
  if (!saveFile.open(QIODevice::WriteOnly)) {
    qWarning("Couldn't open save file.");
    return false;
  }

  QJsonObject themeJson = toJson();
  saveFile.write(QJsonDocument(themeJson).toJson());
  saveFile.close();

#ifdef __EMSCRIPTEN__
  EM_ASM(FS.syncfs(function(err) {
    if (err) console.log(err)
  }););
#endif

  //  Disable save flag
  isDirty_ = false;

  return true;
}

void Theme::loadMissing() {

  prefs_.resize(static_cast<int>(Themes::Roles::Total), nullptr);
  //  Loop through all preferences
  for (int i = 0; i < Themes::Roles::Total; ++i) {

    Preference *pref = nullptr;

    auto id = static_cast<Themes::Roles>(i);

    //  If preference is not null, it was already assigned
    if (prefs_[id] != nullptr) continue;

    //  Current preference is missing. Assign a default.
    switch (i) {
    case Themes::Roles::BaseRole:
      pref =
          new Preference(this, id, "Base Text/Background", qRgb(0x0, 0x0, 0x0), qRgb(0xff, 0xff, 0xff)); //  Black/White
      break;
    case Themes::Roles::WindowRole:
      pref = new Preference(this, id, "Window Text/Background", qRgb(0x0f, 0x0f, 0x0f),
                            qRgb(0xee, 0xee, 0xee)); //  Dark Gray/gray
      break;
    case Themes::Roles::ButtonRole:
      pref = new Preference(this, id, "Button Text/Background", qRgb(0x0, 0x0, 0x0),
                            qRgb(0xf0, 0xf0, 0xf0)); //  Black/gray
      break;
    case Themes::Roles::HighlightRole:
      pref = new Preference(this, id, "Highlight Text/Background", qRgb(0xff, 0xff, 0xff),
                            qRgb(0x0, 0x78, 0xd7)); //  White/Mid Blue
      break;
    case Themes::Roles::TooltipRole:
      pref = new Preference(this, id, "Tooltip Text/Background", qRgb(0x0, 0x0, 0x0),
                            qRgb(0xff, 0xff, 0xdc)); //  black/light yellow
      break;
    case Themes::Roles::AlternateBaseRole:
      pref = new Preference(this, id, "AlternateBase Background", qRgb(0x7f, 0x7f, 0x7f),
                            qRgb(0xa0, 0xa0, 0xa0)); //  Dark Gray/Light gray
      break;
    case Themes::Roles::AccentRole:
      pref = new Preference(this, id, "Accent Background", qRgb(0xff, 0xff, 0xff),
                            qRgb(0x0, 0x78, 0xd7)); //  White/Mid Blue
      break;
    case Themes::Roles::LightRole:
      pref = new Preference(this, id, "Light Background", qRgb(0x0, 0x0, 0xff), qRgb(0xff, 0xff, 0xff)); //  Blue/White
      break;
    case Themes::Roles::MidLightRole:
      pref = new Preference(this, id, "Midlight Background", qRgb(0x00, 0x00, 0x00),
                            qRgb(0xe3, 0xe3, 0xe3)); //  Black/Light Gray
      break;
    case Themes::Roles::MidRole:
      pref = new Preference(this, id, "Mid Background", qRgb(0xf0, 0xf0, 0xf0), qRgb(0xa0, 0xa0, 0xa0)); //  Black/Gray
      break;
    case Themes::Roles::DarkRole:
      pref = new Preference(this, id, "Dark Background", qRgb(0xf0, 0xf0, 0xf0), qRgb(0xa0, 0xa0, 0xa0)); //  Black/Gray
      break;
    case Themes::Roles::ShadowRole:
      pref = new Preference(this, id, "Shadow Background", qRgb(0xff, 0xff, 0xff),
                            qRgb(0x69, 0x69, 0x69)); //  White/Dark Gray
      break;
    case Themes::Roles::LinkRole:
      pref = new Preference(this, id, "Link Text", qRgb(0x0, 0x78, 0xd7), qRgb(0xff, 0xff, 0xff)); //  Mid Blue/White
      break;
    case Themes::Roles::LinkVisitedRole:
      pref = new Preference(this, id, "Link Visited Text", qRgb(0x78, 0x40, 0xa0),
                            qRgb(0xff, 0xff, 0xff)); //  Purple/White
      break;
    case Themes::Roles::BrightTextRole:
      pref = new Preference(this, id, "Bright Text", qRgb(0xff, 0xff, 0xff), qRgb(0xa0, 0xa0, 0xa0)); //  White/Mid
      break;
    case Themes::Roles::PlaceHolderTextRole:
      pref =
          new Preference(this, id, "Placeholder Text", qRgb(0x7f, 0x7f, 0x7f), qRgb(0xee, 0xee, 0xee)); //  Gray/white
      break;
    case Themes::Roles::SymbolRole:
      pref = new Preference(this, id, "Symbol", qRgb(0xb6, 0x7b, 0xbc), qRgba(0xff, 0xff, 0xff, 0xff), 0, false, false,
                            false, false);
      break;
    case Themes::Roles::MnemonicRole:
      pref = new Preference(this, id, "Mnemonic", qRgb(0x25, 0x40, 0xbd), qRgba(0xff, 0xff, 0xff, 0xff), 0, true, false,
                            false, false);
      break;
    case Themes::Roles::DirectiveRole:
      pref = new Preference(this, id, "Directive", qRgb(0x25, 0x40, 0xbd), qRgba(0xff, 0xff, 0xff, 0xff), 0, true,
                            false, false, false);
      break;
    case Themes::Roles::MacroRole:
      pref = new Preference(this, id, "Macro", qRgb(0x25, 0x40, 0xbd), qRgba(0xff, 0xff, 0xff, 0xff), 0, false, true,
                            false, false);
      break;
    case Themes::Roles::CharacterRole:
      pref = new Preference(this, id, "Character", QColor("orangered").rgba(), qRgba(0xff, 0xff, 0xff, 0xff), 0, false,
                            false, false, false);
      break;
    case Themes::Roles::StringRole:
      pref = new Preference(this, id, "String", QColor("orangered").rgba(), qRgba(0xff, 0xff, 0xff, 0xff), 0, false,
                            false, false, false);
      break;
    case Themes::Roles::CommentRole:
      pref = new Preference(this, id, "Comment", qRgb(0x66, 0xa3, 0x33), qRgba(0xff, 0xff, 0xff, 0xff), 0, false, false,
                            false, false);
      break;
    case Themes::Roles::ErrorRole:
      pref = new Preference(this, id, "Error", qRgb(0x00, 0x00, 0x00), qRgba(0xff, 0x00, 0x00, 0xff), 0, false, false,
                            false, false);
      break;
    case Themes::Roles::WarningRole:
      pref = new Preference(this, id, "Warning", qRgb(0x00, 0x00, 0x00), qRgba(0xff, 0xFF, 0xE0, 0xff), 0, false, false,
                            false, false);
      break;
    case Themes::Roles::RowNumberRole:
      pref = new Preference(this, id, "Row Number", qRgb(0x66, 0x66, 0x66), qRgb(0xff, 0xff, 0xff), //  Black/Red
                            0, false, true, false, true); // Italics, Strikeout
      break;
    case Themes::Roles::BreakpointRole:
      pref = new Preference(this, id, "Breakpoint", qRgb(0x00, 0x00, 0x00), qRgb(0xff, 0xaa, 0x00), //  Black/Red
                            0, true, false, false, true);                                           // Bold Strikeout
      break;
    case Themes::Roles::SeqCircuitRole:
      pref = new Preference(this, id, "SeqCircuit", qRgb(0xff, 0xff, 0x00), qRgb(0x04, 0xab, 0x0a), //  Yellow/Green
                            0, false, true);                                                        // Italics
      break;
    case Themes::Roles::CircuitGreenRole:
      pref = new Preference(this, id, "Green Circuit", qRgb(0x0, 0x0, 0xff), qRgb(0xff, 0xe1, 0xff), //  Blue/Violet
                            0, false, false, true);                                                  // Underline
      break;
    default: return;
    }
    pref->setFont(&font_);
    prefs_[id] = pref;
  }
}

QFont Theme::font() const { return font_; }

//  Public QML interface
void Theme::setFont(QFont font) { setFont(font.family(), font.pointSize()); }

//  Internal interface
void Theme::setFont(const QString family, const int pointSize) {

  font_.setFamily(family);

  //  Make sure font is at least 8 points
  font_.setPointSize(std::max(pointSize, 8));

  for (auto &it : prefs_) {
    it->setFont(&font_);
  }
}

QString Theme::name() const { return name_; }

//  Name can only be set on creation. Not changeable in QML.
void Theme::setName(QString name) { name_ = name; }

bool Theme::systemTheme() const { return system_; }

Preference *Theme::preference(int role) {
  //  Cehck error conditions
  if (role < 0 || role >= Themes::Roles::Total) return nullptr;

  //  Role is in our vector, return it
  return prefs_[role];
}

Preference *Theme::preference(int role) const {
  //  Cehck error conditions
  if (role < 0 || role >= Themes::Roles::Total) return nullptr;

  //  Role is in our vector, return it
  return prefs_[role];
}

QStringList Theme::themes() const { return themes_; }
