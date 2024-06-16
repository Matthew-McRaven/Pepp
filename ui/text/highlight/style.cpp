/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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

#include "style.hpp"
using namespace highlight;

Style::Style(QObject *parent) : QObject(parent) {}

QString Style::getFont() const { return m_font; }

void Style::setFont(const QString &newFont) {
  if (m_font == newFont) return;
  m_font = newFont;
  emit fontChanged();
}

QColor Style::getColor() const { return m_color; }

void Style::setColor(const QColor &newColor) {
  if (m_color == newColor) return;
  m_color = newColor;
  emit colorChanged();
}

QColor Style::getBackground() const { return m_background; }

void Style::setBackground(const QColor &newBackground) {
  if (m_background == newBackground) return;
  m_background = newBackground;
  emit backgroundChanged();
}

bool Style::getItalics() const { return m_italics; }

void Style::setItalics(bool newItalics) {
  if (m_italics == newItalics) return;
  m_italics = newItalics;
  emit italicsChanged();
}

QFont::Weight Style::getWeight() const { return m_weight; }

void Style::setWeight(const QFont::Weight &newWeight) {
  if (m_weight == newWeight) return;
  m_weight = newWeight;
  emit weightChanged();
}

bool Style::operator==(const Style &other) const {
  return this->m_font == other.m_font && this->m_color == other.m_color && this->m_background == other.m_background &&
         this->m_italics == other.m_italics && this->m_weight == other.m_weight;
}

QTextCharFormat Style::format() const {
  QTextCharFormat format;
  if (!m_font.isEmpty()) {
    // TODO: Add font support
  }
  if (m_color.isValid()) {
    format.setForeground(m_color);
  }
  if (m_background.isValid()) {
    format.setBackground(m_background);
  }
  format.setFontItalic(m_italics);
  format.setFontWeight(m_weight);
  return format;
}

Q_DECLARE_METATYPE(::highlight::Style *)
