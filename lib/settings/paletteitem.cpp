#include "paletteitem.hpp"
#include <QSet>

pepp::settings::OverrideStateHelper::OverrideStateHelper(QObject *parent) : QObject(parent) {}

pepp::settings::PaletteItem::PaletteItem(PreferenceOptions opts, QObject *parent) : QObject(parent) {
  // WTF to do with opts?
}

pepp::settings::PaletteItem *pepp::settings::PaletteItem::parent() { return _parent; }

const pepp::settings::PaletteItem *pepp::settings::PaletteItem::parent() const { return _parent; }

void pepp::settings::PaletteItem::clearParent() {
  if (_parent) QObject::disconnect(_parent, &PaletteItem::preferenceChanged, this, &PaletteItem::onParentChanged);
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

bool pepp::settings::PaletteItem::hasOwnForeground() const { return _foreground.has_value(); }

bool pepp::settings::PaletteItem::hasOwnBackground() const { return _background.has_value(); }

bool pepp::settings::PaletteItem::hasOwnFont() const { return _font.has_value(); }

pepp::settings::OverrideState pepp::settings::PaletteItem::boldOverride() const {
  if (!_fontOverrides.bold) return OverrideState::Unset;
  else if (_fontOverrides.bold.value()) return OverrideState::True;
  else return OverrideState::False;
}

void pepp::settings::PaletteItem::setBoldOverride(OverrideState bold) {
  if (bold == boldOverride()) return;
  switch (bold) {
  case OverrideStateHelper::OverrideState::True: _fontOverrides.bold = true; break;
  case OverrideStateHelper::OverrideState::False: _fontOverrides.bold = false; break;
  case OverrideStateHelper::OverrideState::Unset: _fontOverrides.bold.reset(); break;
  }
  emit preferenceChanged();
}

pepp::settings::OverrideState pepp::settings::PaletteItem::italicOverride() const {
  if (!_fontOverrides.italic) return OverrideState::Unset;
  else if (_fontOverrides.italic.value()) return OverrideState::True;
  else return OverrideState::False;
}

void pepp::settings::PaletteItem::setItalicOverride(OverrideState italic) {
  if (italic == italicOverride()) return;
  switch (italic) {
  case OverrideStateHelper::OverrideState::True: _fontOverrides.italic = true; break;
  case OverrideStateHelper::OverrideState::False: _fontOverrides.italic = false; break;
  case OverrideStateHelper::OverrideState::Unset: _fontOverrides.italic.reset(); break;
  }
  emit preferenceChanged();
}

pepp::settings::OverrideState pepp::settings::PaletteItem::underlineOverride() const {
  if (!_fontOverrides.underline) return OverrideState::Unset;
  else if (_fontOverrides.underline.value()) return OverrideState::True;
  else return OverrideState::False;
}

void pepp::settings::PaletteItem::setUnderlineOverride(OverrideState underline) {
  if (underline == underlineOverride()) return;
  switch (underline) {
  case OverrideStateHelper::OverrideState::True: _fontOverrides.underline = true; break;
  case OverrideStateHelper::OverrideState::False: _fontOverrides.underline = false; break;
  case OverrideStateHelper::OverrideState::Unset: _fontOverrides.underline.reset(); break;
  }
  emit preferenceChanged();
}

pepp::settings::OverrideState pepp::settings::PaletteItem::strikeoutOverride() const {
  if (!_fontOverrides.strikeout) return OverrideState::Unset;
  else if (_fontOverrides.strikeout.value()) return OverrideState::True;
  else return OverrideState::False;
}

void pepp::settings::PaletteItem::setStrikeoutOverride(OverrideState strikeout) {
  if (strikeout == strikeoutOverride()) return;
  switch (strikeout) {
  case OverrideStateHelper::OverrideState::True: _fontOverrides.strikeout = true; break;
  case OverrideStateHelper::OverrideState::False: _fontOverrides.strikeout = false; break;
  case OverrideStateHelper::OverrideState::Unset: _fontOverrides.strikeout.reset(); break;
  }
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
