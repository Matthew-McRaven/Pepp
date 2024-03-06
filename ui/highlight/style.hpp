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

#pragma once

#include <QQuickItem>
#include <QTextCharFormat>

#include "frontend_globals.hpp"
#include "style/types.hpp"

namespace highlight {
class FRONTEND_EXPORT Style : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString font READ getFont WRITE setFont NOTIFY fontChanged);
  Q_PROPERTY(QColor color READ getColor WRITE setColor NOTIFY colorChanged);
  Q_PROPERTY(QColor background READ getBackground WRITE setBackground NOTIFY
                 backgroundChanged);
  Q_PROPERTY(
      bool italics READ getItalics WRITE setItalics NOTIFY italicsChanged);
  Q_PROPERTY(
      QFont::Weight weight READ getWeight WRITE setWeight NOTIFY weightChanged);

public:
  Style(QObject *parent = nullptr);

  QString getFont() const;
  void setFont(const QString &newFont);

  QColor getColor() const;
  void setColor(const QColor &newColor);

  QColor getBackground() const;
  void setBackground(const QColor &newBackground);

  bool getItalics() const;
  void setItalics(bool newItalics);

  QFont::Weight getWeight() const;
  void setWeight(const QFont::Weight &newWeight);
  bool operator==(const Style &other) const;

  QTextCharFormat format() const;
signals:
  void fontChanged();
  void colorChanged();
  void backgroundChanged();
  void italicsChanged();
  void weightChanged();

private:
  QString m_font;
  QColor m_color;
  QColor m_background;
  bool m_italics = false;
  QFont::Weight m_weight = QFont::Weight::Normal;
};
}; // namespace highlight
