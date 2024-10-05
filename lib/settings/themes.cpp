#include "themes.hpp"

Preference::Preference(const QString name, PreferenceOptions opts, QObject *parent) : QObject(parent), _name(name) {
  _foreground = opts.fg;
  _background = opts.bg;
  _font = opts.font;
  _font.setBold(opts.bold);
  _font.setItalic(opts.italics);
  _font.setUnderline(opts.underline);
  _font.setStrikeOut(opts.strikeOut);
}

QString Preference::name() const { return _name; }

void Preference::setName(const QString name) {
  if (_name == name) return;
  _name = name;
  emit preferenceChanged();
}

QColor Preference::foreground() const { return _foreground; }

void Preference::setForeground(const QColor foreground) {
  if (_foreground == foreground) return;
  _foreground = foreground;
  emit preferenceChanged();
}

QColor Preference::background() const { return _background; }

void Preference::setBackground(const QColor background) {
  if (_background == background) return;
  _background = background;
  emit preferenceChanged();
}

QFont Preference::font() const { return _font; }

void Preference::setFont(const QFont font) {
  if (_font == font) return;
  _font = font;
  emit preferenceChanged();
}

bool Preference::bold() const { return _font.bold(); }

void Preference::setBold(const bool bold) {
  if (_font.bold() == bold) return;
  _font.setBold(bold);
  emit preferenceChanged();
}

bool Preference::italics() const { return _font.italic(); }

void Preference::setItalics(const bool italics) {
  if (_font.italic() == italics) return;
  _font.setItalic(italics);
  emit preferenceChanged();
}

bool Preference::underline() const { return _font.underline(); }

void Preference::setUnderline(const bool underline) {
  if (_font.underline() == underline) return;
  _font.setUnderline(underline);
  emit preferenceChanged();
}

bool Preference::strikeOut() const { return _font.strikeOut(); }

void Preference::setStrikeOut(const bool strikeOut) {
  if (_font.strikeOut() == strikeOut) return;
  _font.setStrikeOut(strikeOut);
  emit preferenceChanged();
}

QJsonObject Preference::toJson() const {
  QJsonObject prefData;
  bool ok;
  quint32 hex;

  prefData["name"] = name();
  hex = static_cast<qint64>(foreground().rgba());
  prefData["foreground"] = QString("0x%1").arg(hex, 8, 16, QLatin1Char('0'));
  hex = static_cast<qint64>(background().rgba());
  prefData["background"] = QString("0x%1").arg(hex, 8, 16, QLatin1Char('0'));
  prefData["bold"] = bold();
  prefData["italics"] = italics();
  prefData["underline"] = underline();
  prefData["strikeOut"] = strikeOut();

  return prefData;
}

bool Preference::updateFromJson(const QJsonObject &json) {
  bool ok;
  if (const QJsonValue v = json["name"]; v.isString()) {
    //  Json is resorted, and this cannot be disabled.
    //  Use enum name for lookup for enum id, and assign
    //  to array in the enum address
    const auto sName = v.toString();
    if (sName.isEmpty()) return false;
    else if (sName != name()) return false;
  }

  if (const QJsonValue v = json["foreground"]; v.isString()) {
    //  Convert from hex string. If error, assign default color
    quint32 color = v.toString().toLongLong(&ok, 16);
    if (ok) _foreground = QRgb(color);
    else _foreground = qRgb(0x0, 0x0, 0x0);
  }
  if (const QJsonValue v = json["background"]; v.isString()) {
    //  Convert from hex string. If error, assign default color
    quint32 color = v.toString().toLongLong(&ok, 16);
    if (ok) _background = QRgb(color);
    else _background = qRgb(0xff, 0xff, 0xff);
  }
  if (const QJsonValue v = json["bold"]; v.isBool()) _font.setBold(v.toBool(false));
  if (const QJsonValue v = json["italics"]; v.isBool()) _font.setItalic(v.toBool(false));
  if (const QJsonValue v = json["underline"]; v.isBool()) _font.setUnderline(v.toBool(false));
  if (const QJsonValue v = json["strikeOut"]; v.isBool()) _font.setStrikeOut(v.toBool(false));

  return true;
}

Preference *Theme::preference(int role) {
  if (role < 0 || role >= (int)Roles::Total) return nullptr;
  return _prefs[role];
}

Preference *Theme::preference(int role) const {

  if (role < 0 || role >= (int)Roles::Total) return nullptr;
  return _prefs[role];
}
Preference *Theme::preference(Roles role) { return preference((int)role); }

Preference *Theme::preference(Roles role) const { return preference((int)role); }

void Theme::resetToDefault() {
  for (auto &pref : _prefs) {
    if (!pref) continue;
    delete pref;
    pref = nullptr;
  }
  for (int i = 0; i < (int)Roles::Total; ++i) _prefs[i] = defaultForRole((Roles)i);
}

Preference *Theme::defaultForRole(Roles role) {
  //  Current preference is missing. Assign a default.
  switch (role) {
  case Roles::BaseRole: return new Preference("Base Text/Background", {},
                                              this); //  Black/White
  case Roles::WindowRole:
    return new Preference("Window Text/Background", {.fg = qRgb(0x0f, 0x0f, 0x0f), .bg = qRgb(0xee, 0xee, 0xee)},
                          this); //  Dark Gray/gray
  case Roles::ButtonRole:
    return new Preference("Button Text/Background", {.bg = qRgb(0xf0, 0xf0, 0xf0)}, this); //  Black/gray
  case Roles::HighlightRole:
    return new Preference("Highlight Text/Background", {.bg = qRgb(0x0, 0x78, 0xd7)}, this); //  White/Mid Blue
  case Roles::TooltipRole:
    return new Preference("Tooltip Text/Background", {.fg = qRgb(0x0, 0x0, 0x0), .bg = qRgb(0xff, 0xff, 0xdc)},
                          this); //  black/light yellow
  case Roles::AlternateBaseRole:
    return new Preference("AlternateBase Background", {.fg = qRgb(0x7f, 0x7f, 0x7f), .bg = qRgb(0xa0, 0xa0, 0xa0)},
                          this); //  Dark Gray/Light gray
  case Roles::AccentRole:
    return new Preference("Accent Background", {.fg = qRgb(0xff, 0xff, 0xff), .bg = qRgb(0x0, 0x78, 0xd7)},
                          this); //  White/Mid Blue
  case Roles::LightRole:
    return new Preference("Light Background", {.fg = qRgb(0x0, 0x0, 0xff), .bg = qRgb(0xff, 0xff, 0xff)},
                          this); //  Blue/White
  case Roles::MidLightRole:
    return new Preference("Midlight Background", {.fg = qRgb(0x00, 0x00, 0x00), .bg = qRgb(0xe3, 0xe3, 0xe3)},
                          this); // Black/Light Gray
  case Roles::MidRole:
    return new Preference("Mid Background", {.fg = qRgb(0xf0, 0xf0, 0xf0), .bg = qRgb(0xa0, 0xa0, 0xa0)},
                          this); // Black/Gray
  case Roles::DarkRole:
    return new Preference("Dark Background", {.fg = qRgb(0xf0, 0xf0, 0xf0), .bg = qRgb(0xa0, 0xa0, 0xa0)},
                          this); // Black/Gray
  case Roles::ShadowRole:
    return new Preference("Shadow Background", {.fg = qRgb(0xff, 0xff, 0xff), .bg = qRgb(0x69, 0x69, 0x69)},
                          this); // White/Dark Gray
  case Roles::LinkRole:
    return new Preference("Link Text", {.fg = qRgb(0x0, 0x78, 0xd7), .bg = qRgb(0xff, 0xff, 0xff)},
                          this); // Mid Blue/White
  case Roles::LinkVisitedRole:
    return new Preference("Link Visited Text", {.fg = qRgb(0x78, 0x40, 0xa0), .bg = qRgb(0xff, 0xff, 0xff)},
                          this); // Purple/White
  case Roles::BrightTextRole:
    return new Preference("Bright Text", {.fg = qRgb(0xff, 0xff, 0xff), .bg = qRgb(0xa0, 0xa0, 0xa0)},
                          this); // White/Mid
  case Roles::PlaceHolderTextRole:
    return new Preference("Placeholder Text", {.fg = qRgb(0x7f, 0x7f, 0x7f), .bg = qRgb(0xee, 0xee, 0xee)},
                          this); //  Gray/white
  case Roles::SymbolRole:
    return new Preference("Symbol", {.fg = qRgb(0xb6, 0x7b, 0xbc), .bg = qRgba(0xff, 0xff, 0xff, 0xff)}, this);
  case Roles::MnemonicRole:
    return new Preference("Mnemonic",
                          {
                              .bold = 1,
                              .fg = qRgb(0x25, 0x40, 0xbd),
                              .bg = qRgba(0xff, 0xff, 0xff, 0xff),
                          },
                          this);
  case Roles::DirectiveRole:
    return new Preference("Directive",
                          {
                              .bold = 1,
                              .fg = qRgb(0x25, 0x40, 0xbd),
                              .bg = qRgba(0xff, 0xff, 0xff, 0xff),
                          },
                          this);
  case Roles::MacroRole:
    return new Preference("Macro",
                          {
                              .italics = 1,
                              .fg = qRgb(0x25, 0x40, 0xbd),
                              .bg = qRgba(0xff, 0xff, 0xff, 0xff),
                          },
                          this);
  case Roles::CharacterRole:
    return new Preference("Character", {.fg = QColor("orangered").rgba(), .bg = qRgba(0xff, 0xff, 0xff, 0xff)}, this);
  case Roles::StringRole:
    return new Preference("String", {.fg = QColor("orangered").rgba(), .bg = qRgba(0xff, 0xff, 0xff, 0xff)}, this);
  case Roles::CommentRole:
    return new Preference("Comment", {.fg = qRgb(0x66, 0xa3, 0x33), .bg = qRgba(0xff, 0xff, 0xff, 0xff)}, this);
  case Roles::ErrorRole:
    return new Preference("Error", {.fg = qRgb(0x00, 0x00, 0x00), .bg = qRgba(0xff, 0x00, 0x00, 0xff)}, this);
  case Roles::WarningRole:
    return new Preference("Warning", {.fg = qRgb(0x00, 0x00, 0x00), .bg = qRgba(0xff, 0xFF, 0xE0, 0xff)}, this);
  case Roles::RowNumberRole:
    return new Preference("Row Number", {.italics = true, .fg = qRgb(0x66, 0x66, 0x66), .bg = qRgb(0xff, 0xff, 0xff)},
                          this); //  Black/Red Italics
  case Roles::BreakpointRole:
    return new Preference("Breakpoint", {.bold = 1, .fg = qRgb(0x00, 0x00, 0x00), .bg = qRgb(0xff, 0xaa, 0x00)},
                          this); //  Black/Red Bold
  case Roles::SeqCircuitRole:
    return new Preference("SeqCircuit", {.fg = qRgb(0xff, 0xff, 0x00), .bg = qRgb(0x04, 0xab, 0x0a)},
                          this); //  Yellow/Green
  case Roles::CircuitGreenRole:
    return new Preference("Green Circuit", {.fg = qRgb(0x0, 0x0, 0xff), .bg = qRgb(0xff, 0xe1, 0xff)},
                          this); //  Blue/Violet
  default: return nullptr;
  }
}

QJsonObject Theme::toJson() const { return {}; }

bool Theme::updateFromJSON(const QJsonObject &json) {
  bool okay;
  static const auto rolesEnum = QMetaEnum::fromType<Roles>();
  // if (const QJsonValue v = json["name"]; v.isString()) _name = v.toString();
  if (auto asInt = json["version"].toInt(0); asInt != _version) {
    qDebug() << "Version mismatch in theme file. Expected " << _version << " got " << asInt;
    return false;
  }
  // Do any global updates, like changing the value of a font for each preference to match global font.
  _name = json["name"].toString();
  if (const QJsonValue v = json["preferences"]; v.isArray()) {
    const QJsonArray prefsObj = v.toArray();
    //  Loop through preferences
    for (const QJsonValue &prefObj : prefsObj) {
      auto asObj = prefObj.toObject();
      if (asObj.isEmpty()) continue;
      auto index = rolesEnum.keyToValue(asObj["id"].toString().toStdString().c_str(), &okay);
      if (!okay || index >= (int)Roles::Total) qWarning() << "Skipping preference: " << asObj["id"].toString();
      else _prefs[index]->updateFromJson(asObj);
    }
  }
  return true;
}

Theme *Theme::fromFile(QString path) {
  //  Read contents and close file
  QFile jsonFile(path);
  if (!jsonFile.open(QIODevice::ReadOnly)) return nullptr;
  QByteArray ba = jsonFile.readAll();
  jsonFile.close();

  QJsonParseError parseError;
  QJsonDocument doc = QJsonDocument::fromJson(ba, &parseError);

  if (parseError.error != QJsonParseError::NoError)
    qWarning() << "Parse error at" << parseError.offset << ":" << parseError.errorString();

  // Implicitly fills self with defaults before being updated in place.
  Theme *theme = new Theme();
  theme->updateFromJSON(doc.object());
  theme->_isDirty = false;
  return theme;
}

Theme::Theme(QObject *parent) : QObject(parent) {
  _prefs.resize((int)Roles::Total);
  resetToDefault();
}

ThemeModel::ThemeModel(QObject *parent) : QAbstractListModel(parent) {
  _themes = loadSystemThemes();
  // loadFromDirectory(MAGIC_USER_LOCATION);
}

int ThemeModel::rowCount(const QModelIndex &parent) const { return _themes.size(); }

QVariant ThemeModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) return {};

  const auto row = index.row();

  switch (role) {
  case Qt::DisplayRole: return QVariant::fromValue(_themes[row]);
  case Qt::UserRole + 1: return QVariant::fromValue(_themes[row]->name());
  case Qt::UserRole + 2: {
  }
  default: return {};
  }

}

Qt::ItemFlags ThemeModel::flags(const QModelIndex &index) const {
  if (!index.isValid() || (std::size_t)index.row() >= _themes.size()) return Qt::NoItemFlags;
  // Prevent removal of system themes.
  else if (_themes[index.row()]->isSystem()) return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  else return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

QHash<int, QByteArray> ThemeModel::roleNames() const {
  static const QHash<int, QByteArray> roles = {
      {Qt::DisplayRole, "display"},
      {Qt::UserRole + 1, "name"},
  };
  return roles;
}

// void ThemeModel::loadFromDirectory(const QString &directory) {}

// void ThemeModel::loadFromJson(const QString &json) {}

QList<Theme *> ThemeModel::loadSystemThemes() {
  QList<Theme *> ret;
  //  Themes shipped with application
  const QDir systemPath(":/themes/");
  QFileInfoList list = systemPath.entryInfoList(QDir::Files);
  for (auto &file : list) {
    if (file.fileName().endsWith(".theme")) {
      Theme *theme = Theme::fromFile(file.absoluteFilePath());
      if (!theme) continue;
      theme->_isSystem = true;
      ret.append(theme);
    }
  }
  return ret;
}

bool ThemeModel::removeRows(int row, int count, const QModelIndex &parent) {
  if (row < 0 || count <= 0 || row + count > _themes.size()) return false;
  // Prevent deleting any system themes
  else if (std::accumulate(_themes[row], _themes[row + count], 0, [](auto a, auto &t) { return a + t.isSystem(); }) > 0)
    return false;
  beginRemoveRows(parent, row, row + count - 1);
  _themes.remove(row, count);
  endRemoveRows();
  return true;
}

CategoryModel::CategoryModel(QObject *parent) : QAbstractListModel(parent) {}

int CategoryModel::rowCount(const QModelIndex &parent) const
{
  static const auto meta = QMetaEnum::fromType<Theme::Category>();
  return meta.keyCount();
}

bool CategoryModel::removeRows(int row, int count, const QModelIndex &parent) { return false; }

QVariant CategoryModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid() || index.row() < 0 || index.row() >= rowCount({}) || index.column() != 0) return {};
  static const auto meta = QMetaEnum::fromType<Theme::Category>();
  switch (role) {
  case Qt::DisplayRole: return QVariant::fromValue(QString(meta.key(index.row())));
  case Qt::UserRole + 1: return QVariant::fromValue(meta.value(index.row()));
  default: return {};
  }
}

Qt::ItemFlags CategoryModel::flags(const QModelIndex &index) const { return Qt::ItemIsEnabled | Qt::ItemIsSelectable; }

QHash<int, QByteArray> CategoryModel::roleNames() const
{
  static const QHash<int, QByteArray> ret = {{Qt::DisplayRole, "display"}, {Qt::UserRole + 1, "value"}};
  return ret;
}

ThemeFilterModel::ThemeFilterModel(QObject *parent) : QSortFilterProxyModel(parent) {}

Theme::Category ThemeFilterModel::category() const { return _cat; }

void ThemeFilterModel::setCategory(Theme::Category category) {
  if (_cat == category) return;
  _cat = category;
  emit categoryChanged();
}

bool ThemeFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
  if (auto sm = sourceModel(); !sm) return false;
  else if (auto asThemeModel = qobject_cast<ThemeModel *>(sm); !asThemeModel) return false;
  else if (auto index = asThemeModel->index(source_row, 0, source_parent); !index.isValid()) return false;
  else if (auto cat = asThemeModel->data(index, Qt::UserRole + 2); !cat.isValid()) return false;
  else if (cat.value<Theme::Category>() != _cat) return false;
  return true;
}
