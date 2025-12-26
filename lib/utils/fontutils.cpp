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

FontUtilsHelper *FontUtilsHelper::nobold() {
  this->_font.setBold(false);
  return this;
}

FontUtilsHelper *FontUtilsHelper::italicize() {
  this->_font.setItalic(true);
  return this;
}

FontUtilsHelper *FontUtilsHelper::noitalicize() {
  this->_font.setItalic(false);
  return this;
}

QFont FontUtilsHelper::font() { return this->_font; }

FontUtils::FontUtils(QObject *parent) : QObject(parent) {}

FontUtilsHelper *FontUtils::fromFont(QFont font) {
  auto ptr = new FontUtilsHelper(font);
  QQmlEngine::setObjectOwnership(ptr, QQmlEngine::JavaScriptOwnership);
  return ptr;
}
