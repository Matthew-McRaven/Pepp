#include "paletteitem.hpp"
#include <QSet>

pepp::settings::PaletteItem::PaletteItem(PreferenceOptions opts, QObject *parent) : QObject(parent) {
  // WTF to do with opts?
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
  if (newParent) QObject::connect(_parent, &PaletteItem::preferenceChanged, this, &PaletteItem::onParentChanged);
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

void pepp::settings::PaletteItem::onParentChanged() { emit preferenceChanged(); }

bool pepp::settings::detail::isAncestorOf(const PaletteItem *maybeAncestor, const PaletteItem *maybeDescendant) {
  QSet<const PaletteItem *> ancestors;
  for (auto ptr = maybeDescendant; ptr != nullptr; ptr = ptr->parent()) {
    ancestors.insert(ptr);
  }
  return ancestors.contains(maybeAncestor);
}
