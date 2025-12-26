/*
 * Copyright (c) 2024-2026 J. Stanley Warford, Matthew McRaven
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "paletteitem.hpp"
#include <QSet>
#include <qfontinfo.h>

pepp::settings::PaletteItem::PaletteItem(Options opts, PaletteRole ownRole, QObject *parent) : QObject(parent) {
  _ownRole = ownRole;
  if (opts.parent) setParent(opts.parent);
  if (opts.fg.has_value()) _foreground = opts.fg;
  if (opts.bg.has_value()) _background = opts.bg;
  if (opts.font.has_value()) updateFont(opts.font.value());
}

pepp::settings::PaletteRole pepp::settings::PaletteItem::ownRole() const { return _ownRole; }

pepp::settings::PaletteItem *pepp::settings::PaletteItem::parent() { return _parent; }

const pepp::settings::PaletteItem *pepp::settings::PaletteItem::parent() const { return _parent; }

void pepp::settings::PaletteItem::clearParent() {
  if (_parent) {
    QObject::disconnect(_parent, &PaletteItem::preferenceChanged, this, &PaletteItem::onParentChanged);
    _foreground = _parent->foreground();
    _background = _parent->background();
    updateFont(_parent->font());
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
  if (PaletteRoleHelper::requiresMonoFont(_ownRole)) preventNonMonoParent();
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
  if (PaletteRoleHelper::requiresMonoFont(_ownRole)) preventNonMonoParent();
  _fontOverrides = {};
  emit preferenceChanged();
}

void pepp::settings::PaletteItem::setFont(const QFont font) {
  if (font == _font) return;
  updateFont(font);
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

bool pepp::settings::PaletteItem::updateFromJson(const QJsonObject &json, PaletteRole ownRole, PaletteItem *parent) {

  _ownRole = ownRole;
  setParent(parent);
  if (json.contains("foreground")) {
    auto hex = json["foreground"].toString().toUInt(nullptr, 16);
    _foreground = QColor::fromRgba(hex);
  } else _foreground.reset();

  if (json.contains("background")) {
    auto hex = json["background"].toString().toUInt(nullptr, 16);
    _background = QColor::fromRgba(hex);
  } else _background = {};

  // If item requires a mono font and the provided font is not mono, reset to a default font.
  if (json.contains("font")) {
    auto font = QFont(json["font"].toString());
    updateFont(font);
    _fontOverrides = {};
  } else {
    _font.reset();
    _fontOverrides = {};
    if (PaletteRoleHelper::requiresMonoFont(_ownRole)) preventNonMonoParent();
    if (json.contains("overrideBold")) _fontOverrides.bold = json["overrideBold"].toBool();
    if (json.contains("overrideItalic")) _fontOverrides.italic = json["overrideItalic"].toBool();
    if (json.contains("overrideUnderline")) _fontOverrides.underline = json["overrideUnderline"].toBool();
    if (json.contains("overrideStrikeout")) _fontOverrides.strikeout = json["overrideStrikeout"].toBool();
    if (json.contains("overrideWeight")) _fontOverrides.weight = json["overrideWeight"].toInt();
  }
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

void pepp::settings::PaletteItem::updateFromSettings(QSettings &settings, PaletteItem *parent) {
  // No gaurantee that the settings object contains anything relevant or even exists.
  // Before resetting all the values, perform a basic sanity test.
  if (!(settings.contains("foreground") || settings.contains("background") || settings.contains("font") ||
        settings.contains("parent")))
    return;
  setParent(parent);
  if (settings.contains("foreground")) {
    auto hex = settings.value("foreground").toUInt();
    _foreground = QColor::fromRgba(hex);
  } else _foreground.reset();

  if (settings.contains("background")) {
    auto hex = settings.value("background").toUInt();
    _background = QColor::fromRgba(hex);
  } else _background.reset();

  if (settings.contains("font")) {
    auto font = QFont(settings.value("font").toString());
    updateFont(font);
    _fontOverrides = {};
  } else {
    _font.reset();
    _fontOverrides = {};
    if (PaletteRoleHelper::requiresMonoFont(_ownRole)) preventNonMonoParent();
    if (settings.contains("overrides")) {
      settings.beginGroup("overrides");
      if (settings.contains("overrideBold")) _fontOverrides.bold = settings.value("overrideBold").toBool();
      if (settings.contains("overrideItalic")) _fontOverrides.italic = settings.value("overrideItalic").toBool();
      if (settings.contains("overrideUnderline"))
        _fontOverrides.underline = settings.value("overrideUnderline").toBool();
      if (settings.contains("overrideStrikeout"))
        _fontOverrides.strikeout = settings.value("overrideStrikeout").toBool();
      if (settings.contains("overrideWeight")) _fontOverrides.weight = settings.value("overrideWeight").toInt();
      settings.endGroup();
    }
  }
  emit preferenceChanged();
}

void pepp::settings::PaletteItem::toSettings(QSettings &settings) const {
  if (hasOwnForeground()) settings.setValue("foreground", foreground().rgba());
  else settings.remove("foreground");

  if (hasOwnBackground()) settings.setValue("background", background().rgba());
  else settings.remove("background");

  if (hasOwnFont()) settings.setValue("font", font().toString());
  else {
    settings.remove("overrides");
    settings.beginGroup("overrides");
    if (_fontOverrides.bold.has_value()) settings.setValue("overrideBold", _fontOverrides.bold.value());
    if (_fontOverrides.italic.has_value()) settings.setValue("overrideItalic", _fontOverrides.italic.value());
    if (_fontOverrides.underline.has_value()) settings.setValue("overrideUnderline", _fontOverrides.underline.value());
    if (_fontOverrides.strikeout.has_value()) settings.setValue("overrideStrikeout", _fontOverrides.strikeout.value());
    if (_fontOverrides.weight.has_value()) settings.setValue("overrideWeight", _fontOverrides.weight.value());
    settings.endGroup();
  }
}

void pepp::settings::PaletteItem::onParentChanged() {
  // If parent font change would violate monospace requirements, reset to a default.
  if (PaletteRoleHelper::requiresMonoFont(_ownRole)) preventNonMonoParent();
  emit preferenceChanged();
}

void pepp::settings::PaletteItem::emitChanged() { emit preferenceChanged(); }

void pepp::settings::PaletteItem::updateFont(const QFont newFont) {
  QFontInfo fontInfo(newFont);
  if (!fontInfo.fixedPitch() && PaletteRoleHelper::requiresMonoFont(_ownRole)) _font = default_mono();
  else {
    _font = newFont;
    _font->setFeature("cv01", 1);
  }
}

void pepp::settings::PaletteItem::preventNonMonoParent() {
  // We either have a font and do not need to care about our parent or we do not care because we don't need a mono font.
  if (hasOwnFont() || !PaletteRoleHelper::requiresMonoFont(_ownRole)) {
  } else if (!_parent) {
    if (!hasOwnFont()) _font = default_mono();
  } else {
    // For some reason, the actual font (returned below) does not set fixed pitch, while QFontInfo does.
    auto font = _parent->font();
    QFontInfo fontInfo(font);
    if (!fontInfo.fixedPitch()) _font = default_mono();
  }
}

bool pepp::settings::detail::isAncestorOf(const PaletteItem *maybeAncestor, const PaletteItem *maybeDescendant) {
  QSet<const PaletteItem *> ancestors;
  for (auto ptr = maybeDescendant; ptr != nullptr; ptr = ptr->parent()) {
    ancestors.insert(ptr);
  }
  return ancestors.contains(maybeAncestor);
}

pepp::settings::EditorPaletteItem::EditorPaletteItem(EditorOptions opts, Options base, PaletteRole ownRole,
                                                     QObject *parent)
    : PaletteItem(base, ownRole, parent), _macroFont(opts.macroFont) {}

QFont pepp::settings::EditorPaletteItem::macroFont() const {
  auto parentAsEditor = dynamic_cast<const EditorPaletteItem *>(_parent);

  if (parentAsEditor && !_macroFont.has_value()) {
    auto baseline = parentAsEditor->macroFont();
    baseline.setBold(_fontOverrides.bold.value_or(baseline.bold()));
    baseline.setItalic(_fontOverrides.italic.value_or(baseline.italic()));
    baseline.setUnderline(_fontOverrides.underline.value_or(baseline.underline()));
    baseline.setStrikeOut(_fontOverrides.strikeout.value_or(baseline.strikeOut()));
    return baseline;
  } else return _macroFont.value_or(default_mono());
};

void pepp::settings::EditorPaletteItem::clearMacroFont() {
  _macroFont.reset();
  emit preferenceChanged();
}

bool pepp::settings::EditorPaletteItem::hasOwnMacroFont() const { return _macroFont.has_value(); }

void pepp::settings::EditorPaletteItem::setMacroFont(const QFont font) {
  if (font == _macroFont) return;
  updateMacroFont(font);
  emit preferenceChanged();
}

bool pepp::settings::EditorPaletteItem::updateFromJson(const QJsonObject &json, PaletteRole ownRole,
                                                       PaletteItem *parent) {
  if (json.contains("macroFont")) updateMacroFont(QFont(json["macroFont"].toString()));

  return PaletteItem::updateFromJson(json, ownRole, parent);
}

QJsonObject pepp::settings::EditorPaletteItem::toJson() {
  auto ret = PaletteItem::toJson();
  if (_macroFont.has_value()) ret["macroFont"] = _macroFont->toString();
  return ret;
}

void pepp::settings::EditorPaletteItem::updateFromSettings(QSettings &settings, PaletteItem *parent) {
  PaletteItem::updateFromSettings(settings, parent);
  if (settings.contains("macroFont")) {
    auto font = QFont(settings.value("macroFont").toString());
    updateMacroFont(font);
    _fontOverrides = {};
  }
}

void pepp::settings::EditorPaletteItem::toSettings(QSettings &settings) const {
  PaletteItem::toSettings(settings);
  if (hasOwnMacroFont()) settings.setValue("macroFont", macroFont().toString());
}

void pepp::settings::EditorPaletteItem::updateMacroFont(const QFont newFont) {
  QFontInfo fontInfo(newFont);
  if (!fontInfo.fixedPitch()) _macroFont = default_mono();
  else _macroFont = newFont;
}

namespace {
auto create = []() {
  auto f = QFont("Monaspace Argon", 12);
  f.setFeature("cv01", 1);
  return f;
};
} // namespace
QFont pepp::settings::default_mono() {
  // Use a helper to only initialize the font once
  static const QFont mono = create();
  return mono;
}
