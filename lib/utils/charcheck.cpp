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
#include "charcheck.hpp"

#include <QFontMetrics>

CharCheck::CharCheck(QObject *parent) : QObject{parent} {}

bool CharCheck::isCharSupported(const QString &character, const QFont &font) {
  if (character.isEmpty())
    return true;
  return QFontMetrics(font).inFont(character.at(0));
}

QFont CharCheck::noMerge(const QFont &font) {
  QFont ret = font;
  ret.setStyleStrategy(QFont::StyleStrategy::NoFontMerging);
  return ret;
}
