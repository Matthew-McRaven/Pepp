#include "paletteitem.hpp"
#include <QSet>

pepp::settings::PaletteItem::PaletteItem(PreferenceOptions opts, QObject *parent) : QObject(parent) {
  if (opts.parent) setParent(opts.parent);
  if (opts.fg.has_value()) _foreground = opts.fg;
  if (opts.bg.has_value()) _background = opts.bg;
  if (opts.font.has_value()) _font = opts.font;
}

pepp::settings::PaletteItem *pepp::settings::PaletteItem::parent() { return _parent; }

const pepp::settings::PaletteItem *pepp::settings::PaletteItem::parent() const { return _parent; }

void pepp::settings::PaletteItem::clearParent() {
  if (_parent) {
    QObject::disconnect(_parent, &PaletteItem::preferenceChanged, this, &PaletteItem::onParentChanged);
    _foreground = _parent->foreground();
    _background = _parent->background();
    _font = _parent->font();
    _fontOverrides = {};
  }

  _parent = nullptr;
  emit preferenceChanged();
}

void pepp::settings::PaletteItem::setParent(PaletteItem *newParent) {
  if (newParent == _parent) return;
  else if (detail::isAncestorOf(this, newParent)) return;
  if (_parent) QObject::disconnect(_parent, &PaletteItem::preferenceChanged, this, &PaletteItem::onParentChanged);
  _parent = newParent;
  if (_parent) QObject::connect(_parent, &PaletteItem::preferenceChanged, this, &PaletteItem::onParentChanged);
  emit preferenceChanged();
}

QColor pepp::settings::PaletteItem::foreground() const {
  if (_parent && !_foreground.has_value()) return _parent->foreground();
  else return _foreground.value_or(QColor{Qt::white});
}

void pepp::settings::PaletteItem::clearForeground() {
  if (!_foreground.has_value()) return;
  _foreground.reset();
  emit preferenceChanged();
}

void pepp::settings::PaletteItem::setForeground(const QColor foreground) {
  if (foreground == _foreground) return;
  _foreground = foreground;
  emit preferenceChanged();
}

QColor pepp::settings::PaletteItem::background() const {
  if (_parent && !_background.has_value()) return _parent->background();
  else return _background.value_or(QColor{Qt::black});
}

void pepp::settings::PaletteItem::clearBackground() {
  if (!_background.has_value()) return;
  _background.reset();
  emit preferenceChanged();
}

void pepp::settings::PaletteItem::setBackground(const QColor background) {
  if (background == _background) return;
  _background = background;
  emit preferenceChanged();
}

QFont pepp::settings::PaletteItem::font() const {
  if (_parent && !_font.has_value()) {
    auto baseline = _parent->font();
    baseline.setBold(_fontOverrides.bold.value_or(baseline.bold()));
    baseline.setItalic(_fontOverrides.italic.value_or(baseline.italic()));
    baseline.setUnderline(_fontOverrides.underline.value_or(baseline.underline()));
    baseline.setStrikeOut(_fontOverrides.strikeout.value_or(baseline.strikeOut()));
    return baseline;
  } else return _font.value_or(QFont{});
}

void pepp::settings::PaletteItem::clearFont() {
  _font.reset();
  _fontOverrides = {};
  emit preferenceChanged();
}

void pepp::settings::PaletteItem::setFont(const QFont font) {
  if (font == _font) return;
  _font = font;
  emit preferenceChanged();
}

bool pepp::settings::PaletteItem::hasOwnForeground() const { return !_parent || _foreground.has_value(); }

bool pepp::settings::PaletteItem::hasOwnBackground() const { return !_parent || _background.has_value(); }

bool pepp::settings::PaletteItem::hasOwnFont() const { return !_parent || _font.has_value(); }

void pepp::settings::PaletteItem::overrideBold(bool bold) {
  _fontOverrides.bold = bold;
  emit preferenceChanged();
}

void pepp::settings::PaletteItem::overrideItalic(bool italic) {
  _fontOverrides.italic = italic;
  emit preferenceChanged();
}

void pepp::settings::PaletteItem::overrideUnderline(bool underline) {
  _fontOverrides.underline = underline;
  emit preferenceChanged();
}

void pepp::settings::PaletteItem::overrideStrikeout(bool strikeout) {
  _fontOverrides.strikeout = strikeout;
  emit preferenceChanged();
}

bool pepp::settings::PaletteItem::updateFromJson(const QJsonObject &json, PaletteItem *parent) {

  if (json.contains("foreground")) {
    auto hex = json["foreground"].toString().toUInt(nullptr, 16);
    _foreground = QColor::fromRgba(hex);
  }
  if (json.contains("background")) {
    auto hex = json["background"].toString().toUInt(nullptr, 16);
    _background = QColor::fromRgba(hex);
  }
  if (json.contains("font")) {
    _font = QFont(json["font"].toString());
  } else {
    if (json.contains("overrideBold")) _fontOverrides.bold = json["overrideBold"].toBool();
    if (json.contains("overrideItalic")) _fontOverrides.italic = json["overrideItalic"].toBool();
    if (json.contains("overrideUnderline")) _fontOverrides.underline = json["overrideUnderline"].toBool();
    if (json.contains("overrideStrikeout")) _fontOverrides.strikeout = json["overrideStrikeout"].toBool();
    if (json.contains("overrideWeight")) _fontOverrides.weight = json["overrideWeight"].toInt();
  }

  if (parent != nullptr) setParent(parent);
  return true;
}

QJsonObject pepp::settings::PaletteItem::toJson() {
  QJsonObject prefData;
  quint32 hex;
  // We don't know how to convert our parent pointer to a enum, let Palette do this on our behalf.
  if (hasOwnForeground()) {
    hex = static_cast<qint64>(foreground().rgba());
    prefData["foreground"] = QString("0x%1").arg(hex, 8, 16, QLatin1Char('0'));
  }
  if (hasOwnBackground()) {
    hex = static_cast<qint64>(background().rgba());
    prefData["background"] = QString("0x%1").arg(hex, 8, 16, QLatin1Char('0'));
  }
  if (hasOwnFont()) {
    prefData["font"] = font().toString();
  } else {
    if (_fontOverrides.bold.has_value()) prefData["overrideBold"] = _fontOverrides.bold.value();
    if (_fontOverrides.italic.has_value()) prefData["overrideItalic"] = _fontOverrides.italic.value();
    if (_fontOverrides.underline.has_value()) prefData["overrideUnderline"] = _fontOverrides.underline.value();
    if (_fontOverrides.strikeout.has_value()) prefData["overrideStrikeout"] = _fontOverrides.strikeout.value();
    if (_fontOverrides.weight.has_value()) prefData["overrideWeight"] = _fontOverrides.weight.value();
  }

  return prefData;
}

void pepp::settings::PaletteItem::onParentChanged() { emit preferenceChanged(); }

void pepp::settings::PaletteItem::emitChanged() { emit preferenceChanged(); }

bool pepp::settings::detail::isAncestorOf(const PaletteItem *maybeAncestor, const PaletteItem *maybeDescendant) {
  QSet<const PaletteItem *> ancestors;
  for (auto ptr = maybeDescendant; ptr != nullptr; ptr = ptr->parent()) {
    ancestors.insert(ptr);
  }
  return ancestors.contains(maybeAncestor);
}
