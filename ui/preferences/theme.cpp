#include "theme.hpp"

#include <QCoreApplication> //  For application executable path
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

Theme::Theme(QObject *parent)
    : QObject{parent}
    , font_("Courier New", 12)
{
  prefs_.reserve(Roles::Total);

  //  Read system themes from QRC file
  systemPath_ =  ":/themes/";

  //  Location for user themes
  userPath_ = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)+ "/";

  //  Load system and user themes
  loadThemeList();

  //  See if themes were found
  if(themes_.size() == 0)
    //  User deleted system themes, load defaults
    loadMissing();
  else
    //  In future, this will load last saved theme. For now, it's hardcoded
    load(systemPath_ + "Default.theme");

    //  Used to generate sample file
    //save("Default.theme");
}

void Theme::loadThemeList() {
  QDir dir;

  //  Themes shipped with application
  dir.setPath(systemPath_);
  QFileInfoList list = dir.entryInfoList(QDir::Files);
  for( auto& file : list ) {
    if(file.fileName().endsWith(".theme"))
      themes_.append(file.baseName());
  }

  //  User themes-can be updated
  dir.setPath(userPath_);
  list = dir.entryInfoList(QDir::Files);
  for( auto& file : list ) {
    if(file.fileName().endsWith(".theme"))
      themes_.append(file.baseName());
  }
}

void Theme::load(const QString& file) {
    QFile jsonFile(file);

    QJsonParseError parseError;

    if(!jsonFile.open(QIODevice::ReadOnly)) {
        //  File could not be opened, load default if empty
        if(prefs_.size() == 0)
            loadMissing();

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
}

void Theme::fromJson(const QJsonObject &json)
{
  if (const QJsonValue v = json["name"]; v.isString())
    name_ = v.toString();
  if (const QJsonValue v = json["version"]; v.isString())
    //assert( version_ == v.toString());
    version_ = v.toString();
  if (const QJsonValue v = json["system"]; v.isBool())
    system_ = v.toBool();
  if (const QJsonValue v = json["name"]; v.isString())
    font_.setFamily(v.toString());
  if (const QJsonValue v = json["points"]; v.isDouble())
     font_.setPointSize(v.toInt());

  //  Remove previous preferences
  prefs_.clear();

  if (const QJsonValue v = json["preferences"]; v.isArray()) {
    const QJsonArray prefsObj = v.toArray();

    //  Set size of preferences equal to items in file
    prefs_.resize(prefsObj.size());

    //  Loop through preferences
    for(const QJsonValue &prefObj : prefsObj) {
      //  Create new preference
      Preference* pref  = new Preference(this);

      //  Initialize with font from above
      pref->setFont(&font_);

      //  Populate from json object
      Preference::fromJson(prefObj.toObject(), *pref);
      Q_ASSERT(pref->id() < Roles::Total);

      //  Json is resorted, and this cannot be disabled.
      //  Use ID to save in correct place in theme array
      prefs_[pref->id()]=pref;
    }
  }
}

//  Callback from QML
void Theme::selectTheme(const QString newTheme) {
  //  If same theme, no changes required
  if(newTheme == name_)
    return;

  for( const auto& theme : themes_) {
    if(newTheme == theme )
    {
      QString systemFile = systemPath_ + theme + ".theme";
      QString userFile = userPath_ + theme + ".theme";
      //  Search system first
      if(QFile::exists(systemFile)) {
        load(systemFile);
        break;
      }
      else if(QFile::exists(userFile)) {
        load(userFile);
        break;
      }
    }
  }
  //  Notify QML that theme has changed
  emit fontChanged();
  emit preferenceChanged();
  emit themesChanged();
}

void Theme::exportTheme(const QString file) const {
  auto cleanFile = file;

  //  Remove "file:///" from URL file name
  cleanFile = cleanFile.remove(0,8);

  save(cleanFile);
}

void Theme::importTheme(const QString file) {
  auto cleanFile = file;

  //  Remove "file:///" from URL file name
  cleanFile = cleanFile.remove(0,8);

  load(cleanFile);

  //  Notify QML that theme has changed
  emit fontChanged();
  emit preferenceChanged();
  emit themesChanged();
}

void Theme::copyTheme(const QString theme) {
  //  Store in user data area
  QString fullName = userPath_ + theme + ".theme";

  //  Update theme name to match user update
  name_ = theme;

         //  Change to non-system theme
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

  if(!themeFile.exists()) {
    //  Theme file cannot be found. Just return
    return;
  }

  //  Delete old theme file and entry
  themeFile.remove();
  themes_.removeAt(themes_.indexOf(theme));

  //  Load Default theme
  const QString systemFile = systemPath_ + "Default.theme";
  //  Search system first
  if(QFile::exists(systemFile)) {
    load(systemFile);
  }
  else
    //  No theme files. Create default theme
    loadMissing();

  //  Let the world know that there is a new theme
  emit fontChanged();
  emit preferenceChanged();
  emit themesChanged();
}

bool Theme::save(const QString& file) const {
  QFile saveFile (file);

  if (!saveFile.open(QIODevice::WriteOnly)) {
      qWarning("Couldn't open save file.");
      return false;
  }

  QJsonObject themeJson = toJson();
  saveFile.write(QJsonDocument(themeJson).toJson());
  saveFile.close();

  return true;
}

QJsonObject Theme::toJson() const {
  QJsonObject doc;

  //  Save font structure to document
  doc["name"] = name_;
  doc["version"] = version_;
  doc["system"] = system_;

  QJsonObject fontData;
  fontData["name"]    = font_.family();
  fontData["points"]  = font_.pointSize();

  //  Save font structure to document
  doc["font"] = fontData;

  QJsonArray prefData;

  //  Save individual preferences to an array
  for(const auto* p : prefs_) {
    prefData.append( p->toJson());
  }

  //  Append preferences to document
  doc["preferences"] = prefData;

  return doc;
}

void Theme::loadMissing() {

  //  This function is only called if all themes have been removed from the system
  themes_.append("Default");

  //  Clear old preferences before reloading
  prefs_.clear();

  for(int i = 0; i < Roles::Total; ++i ) {

    Preference* pref = nullptr;

    switch(i) {
    case Roles::BaseRole:
      pref  = new Preference(this, BaseRole, "Base Text/Background",
              qRgb(0x0,0x0,0x0),qRgb(0xff,0xff,0xff));  //  Black/White
      break;
    case Roles::WindowRole:
      pref =  new Preference(this, WindowRole, "Window Text/Background",
              qRgb(0x0f,0x0f,0x0f),qRgb(0xee,0xee,0xee));  //  Dark Gray/gray
      break;
    case Roles::ButtonRole:
      pref =  new Preference(this, ButtonRole, "Button Text/Background",
              qRgb(0x0,0x0,0x0),qRgb(0xf0,0xf0,0xf0)); //  Black/gray
      break;
    case Roles::HighlightRole:
      pref =  new Preference(this, HighlightRole, "Highlight Text/Background",
              qRgb(0xff,0xff,0xff),qRgb(0x0,0x78,0xd7)); //  White/Mid Blue
      break;
    case Roles::TooltipRole:
      pref =  new Preference(this, TooltipRole, "Tooltip Text/Background",
              qRgb(0x0,0x0,0x0),qRgb(0xff,0xff,0xdc)); //  black/light yellow
      break;
    case Roles::AlternateBaseRole:
      pref =  new Preference(this, AlternateBaseRole, "AlternateBase Background",
              qRgb(0x7f,0x7f,0x7f),qRgb(0xa0,0xa0,0xa0)); //  Dark Gray/Light gray
      break;
    case Roles::AccentRole:
      pref =  new Preference(this, AccentRole,"Accent Background",
              qRgb(0xff,0xff,0xff),qRgb(0x0,0x78,0xd7)); //  White/Mid Blue
      break;
    case Roles::LightRole:
      pref =  new Preference(this, LightRole,"Light Background",
              qRgb(0x0,0x0,0xff),qRgb(0xff,0xff,0xff)); //  Blue/White
      break;
    case Roles::MidLightRole:
      pref =  new Preference(this, MidLightRole,"Midlight Background",
              qRgb(0x00,0x00,0x00),qRgb(0xe3,0xe3,0xe3)); //  Black/Light Gray
      break;
    case Roles::MidRole:
      pref =  new Preference(this, MidRole,"Mid Background",
              qRgb(0xf0,0xf0,0xf0),qRgb(0xa0,0xa0,0xa0)); //  Black/Gray
      break;
    case Roles::DarkRole:
      pref =  new Preference(this, DarkRole,"Dark Background",
              qRgb(0xf0,0xf0,0xf0),qRgb(0xa0,0xa0,0xa0)); //  Black/Gray
      break;
    case Roles::ShadowRole:
      pref =  new Preference(this, ShadowRole,"Shadow Background",
              qRgb(0xff,0xff,0xff),qRgb(0x69,0x69,0x69)); //  White/Dark Gray
      break;
    case Roles::LinkRole:
      pref =  new Preference(this, LinkRole,"Link Text",
              qRgb(0x0,0x78,0xd7),qRgb(0xff,0xff,0xff)); //  Mid Blue/White
      break;
    case Roles::LinkVisitedRole:
      pref =  new Preference(this, LinkVisitedRole,"Link Visited Text",
              qRgb(0x78,0x40,0xa0),qRgb(0xff,0xff,0xff)); //  Purple/White
      break;
    case Roles::BrightTextRole:
      pref =  new Preference(this, BrightTextRole,"Bright Text",
              qRgb(0xff,0xff,0xff),qRgb(0xa0,0xa0,0xa0)); //  White/Mid
      break;
    case Roles::PlaceHolderTextRole:
      pref =  new Preference(this, PlaceHolderTextRole,"Placeholder Text",
              qRgb(0x7f,0x7f,0x7f),qRgb(0xee,0xee,0xee)); //  Gray/white
      break;
    case Roles::RowNumberRole:
      pref =  new Preference(this, RowNumberRole,"Row Number",
              qRgb(0x66,0x66,0x66),qRgb(0xff,0xff,0xff), //  Black/Red
              0, false, false, false, true); // Strikeout
        break;

    case Roles::BreakpointRole:
        pref =  new Preference(this, BreakpointRole,"Breakpoint",
                qRgb(0x00,0x00,0x00),qRgb(0xff,0xaa,0x00), //  Black/Red
                0, false, false, false, true); // Strikeout
      break;

    case Roles::SeqCircuitRole:
        pref =  new Preference(this, SeqCircuitRole,"SeqCircuit",
                qRgb(0xff,0xff,0x00),qRgb(0x04,0xab,0x0a), //  Yellow/Green
                0); // None
      break;

    case Roles::CircuitGreenRole:
        pref =  new Preference(this, CircuitGreenRole,"Green Circuit",
                qRgb(0x0,0x0,0xff),qRgb(0xff,0xe1,0xff), //  Blue/Violet
                0); // None
        break;
    default:
        return;
    }
    pref->setFont(&font_);
    prefs_.emplace_back(pref);
  }
}

QFont Theme::font() const {
    return font_;
}

void Theme::setFont(QFont font) {
  font_.setFamily(font.family());

  //  Make sure font is at least 8 points
  font_.setPointSize(std::max(font.pointSize(), 8));

  for (auto* it : prefs_) {
    it->setFont(&font_);
  }
}

QString Theme::name() const {
  return name_;
}

//  Name can only be set on creation. Not changeable in QML.
void Theme::setName(QString name) {
  name_ = name;
}

bool Theme::systemTheme() const {
  return system_;
}

Preference* Theme::preference(int role) {
  //  Cehck error conditions
  if( role < 0 || role >= Roles::Total)
    return nullptr;

  //  Role is in our vector, return it
  return prefs_[role];
}

Preference* Theme::preference(int role) const {
  //  Cehck error conditions
  if( role < 0 || role >= Roles::Total)
    return nullptr;

         //  Role is in our vector, return it
  return prefs_[role];
}

QStringList Theme::themes() const {
  return themes_;
}
