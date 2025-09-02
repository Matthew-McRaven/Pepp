#include "fontutils.hpp"

#include <QQmlEngine>

FontUtilsHelper::FontUtilsHelper(QFont font, QObject *parent) : QObject(parent), _font(font) {}

FontUtilsHelper *FontUtilsHelper::h1() {
  this->_font.setPointSizeF(this->_font.pointSizeF() * 2);
  return this;
}

FontUtilsHelper *FontUtilsHelper::h2() {
  this->_font.setPointSizeF(this->_font.pointSizeF() * 1.5);
  return this;
}

FontUtilsHelper *FontUtilsHelper::h3() {
  this->_font.setPointSizeF(this->_font.pointSizeF() * 1.17);
  return this;
}

FontUtilsHelper *FontUtilsHelper::bold() {
  this->_font.setBold(true);
  return this;
}

FontUtilsHelper *FontUtilsHelper::italicize() {
  this->_font.setItalic(true);
  return this;
}

QFont FontUtilsHelper::font() { return this->_font; }

FontUtils::FontUtils(QObject *parent) : QObject(parent) {}

FontUtilsHelper *FontUtils::fromFont(QFont font) {
  auto ptr = new FontUtilsHelper(font);
  QQmlEngine::setObjectOwnership(ptr, QQmlEngine::JavaScriptOwnership);
  return ptr;
}
