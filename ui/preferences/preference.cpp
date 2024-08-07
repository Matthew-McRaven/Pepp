#include "preference.hpp"

#include <QJsonObject>
#include <QJsonValue>
#include <QMetaEnum>

#include "themes.hpp"
Preference::Preference(QObject *parent) : QObject(parent), d(new PreferencePrivate) {}

Preference::Preference(QObject *parent, const Themes::Roles id, const QString name)
    : QObject(parent), d(new PreferencePrivate) {
  d->id_ = id;
  d->name_ = name;
}

Preference::Preference(QObject *parent, const Themes::Roles id, const QString name, const QRgb foreground,
                       const QRgb background, const quint32 parentId, const bool bold, const bool italics,
                       const bool underline, const bool strikeout)
    : QObject(parent), d(new PreferencePrivate) {
  d->id_ = id;
  d->name_ = name;
  d->foreground_ = QRgba64::fromArgb32(foreground);
  d->background_ = QRgba64::fromArgb32(background);
  d->parentId_ = parentId;

  d->font_.setBold(bold);
  d->font_.setItalic(italics);
  d->font_.setUnderline(underline);
  d->font_.setStrikeOut(strikeout);
}

size_t Preference::size() const { return 11; }

//  Getter & Setter
Themes::Roles Preference::id() const { return d->id_; }

QString Preference::name() const { return d->name_; }

int Preference::parentId() const { return d->parentId_; }

QColor Preference::foreground() const { return d->foreground_; }

QColor Preference::background() const { return d->background_; }

QFont Preference::font() const { return d->font_; }

bool Preference::bold() const { return d->font_.bold(); }

bool Preference::italics() const { return d->font_.italic(); }

bool Preference::underline() const { return d->font_.underline(); }

bool Preference::strikeOut() const { return d->font_.strikeOut(); }

void Preference::setId(const Themes::Roles id) { d->id_ = id; }

void Preference::setName(const QString name) { d->name_ = name; }

void Preference::setParent(const quint32 parent) { d->parentId_ = parent; }

void Preference::setForeground(const QColor foreground) { d->foreground_ = foreground; }

void Preference::setBackground(const QColor background) { d->background_ = background; }

void Preference::setFont(QFont *font) {
  //  Only using font family and pointsize. If same, font has not changed
  if (d->font_.family() == font->family() && d->font_.pointSize() == font->pointSize()) return;

  d->font_.setFamily(font->family());
  //  Font must always be 8 points or larger
  d->font_.setPointSize(std::max(font->pointSize(), 8));
}

void Preference::setBold(const bool bold) { d->font_.setBold(bold); }

void Preference::setItalics(const bool italics) { d->font_.setItalic(italics); }

void Preference::setUnderline(const bool underline) { d->font_.setUnderline(underline); }

void Preference::setStrikeOut(const bool strikeOut) { d->font_.setStrikeOut(strikeOut); }

QJsonObject Preference::toJson() const {
  QJsonObject prefData;
  bool ok;
  quint32 hex;

  auto key = QMetaEnum::fromType<Themes::Roles>().valueToKey(id());
  prefData["id"] = key; // id();
  prefData["name"] = name();
  prefData["parent"] = parentId();
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

bool Preference::fromJson(const QJsonObject &json, Preference &pref) {

  bool ok;
  if (const QJsonValue v = json["id"]; v.isString()) {
    //  Json is resorted, and this cannot be disabled.
    //  Use enum name for lookup for enum id, and assign
    //  to array in the enum address

    //  Check for conversion errors
    const auto sName = v.toString();
    if (sName.isEmpty()) return false;

    const auto stdName = sName.toStdString();
    if (stdName.empty()) return false;

    const char *name = stdName.c_str();
    if (name == nullptr) return false;

    auto id = static_cast<Themes::Roles>(QMetaEnum::fromType<Themes::Roles>().keyToValue(name));

    pref.setId(id);
  }
  if (const QJsonValue v = json["name"]; v.isString()) pref.setName(v.toString());
  if (const QJsonValue v = json["parent"]; v.isDouble()) pref.setParent(v.toInt());
  if (const QJsonValue v = json["foreground"]; v.isString()) {
    //  Convert from hex string. If error, assign default color
    quint32 color = v.toString().toLongLong(&ok, 16);
    if (ok) pref.setForeground(QRgb(color));
    else pref.setForeground(qRgb(0x0, 0x0, 0x0));
  }
  if (const QJsonValue v = json["background"]; v.isString()) {
    //  Convert from hex string. If error, assign default color
    quint32 color = v.toString().toLongLong(&ok, 16);
    if (ok) pref.setBackground(QRgb(color));
    else pref.setBackground(qRgb(0xff, 0xff, 0xff));
  }
  if (const QJsonValue v = json["bold"]; v.isBool()) pref.setBold(v.toBool(false));
  if (const QJsonValue v = json["italics"]; v.isBool()) pref.setItalics(v.toBool(false));
  if (const QJsonValue v = json["underline"]; v.isBool()) pref.setUnderline(v.toBool(false));
  if (const QJsonValue v = json["strikeOut"]; v.isBool()) pref.setStrikeOut(v.toBool(false));

  return true;
}
