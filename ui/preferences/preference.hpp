/*
 * Copyright (c) 2024 J. Stanley Warford, Matthew McRaven
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

#ifndef PREFERENCE_HPP
#define PREFERENCE_HPP

#include <QColor>
#include <QFont>
#include <QObject>
#include "frontend_globals.hpp"

class FRONTEND_EXPORT Preference {
  Q_GADGET
  Q_PROPERTY(int id         READ id)
  Q_PROPERTY(QString name   READ name)
  Q_PROPERTY(QColor foreground READ foreground WRITE setForeground)
  Q_PROPERTY(QColor background READ background WRITE setBackground)
  Q_PROPERTY(QFont font     READ font)
  Q_PROPERTY(bool bold      READ bold      WRITE setBold);
  Q_PROPERTY(bool italics   READ italics   WRITE setItalics);
  Q_PROPERTY(bool underline READ underline WRITE setUnderline);
  Q_PROPERTY(bool strikeOut READ strikeOut WRITE setStrikeOut);

  quint32 id_ = 0;
  quint32 parentId_ = 0;
  QString name_{};
  quint32 type_ = 0;
  QColor foreground_{Qt::black};
  QColor background_{Qt::white};
  QFont font_;

public:
  Preference() = default;
  ~Preference() = default;

  Preference(const quint32 id, const QString name, const quint32 type);

  Preference(const quint32 id, const QString name, const quint32 type, const quint32 parentId, const QRgb foreground,
             const QRgb background, const bool bold = false, const bool italics = false, const bool underline = false,
             const bool strikeOut = false);

  //	Copying Ok
  Preference(const Preference &) = default;
  Preference &operator=(const Preference &) = default;
  //	Moving OK
  Preference(Preference &&) noexcept = default;
  Preference &operator=(Preference &&) = default;

  size_t size() const { return 11; }

  //  Getter & Setter
  quint32 id() const { return id_; }
  QString name() const { return name_; }

  quint32 parentId() const { return parentId_; }
  quint32 type() const { return type_; }
  QColor foreground() const { return foreground_; }
  QColor background() const { return background_; }
  QFont font() const { return font_; }

  bool bold()      const { return font_.bold(); }
  bool italics()   const { return font_.italic(); }
  bool underline() const { return font_.underline(); }
  bool strikeOut() const { return font_.strikeOut(); }

  void setParent(const quint32 parent) { parentId_ = parent; }
  void setForeground(const QColor foreground) { foreground_ = foreground; }
  void setBackground(const QColor background) { background_ = background; }
  void setFont(QFont *font) {
    if (font_.family() == font->family() && font_.pointSize() == font->pointSize())
      return;

    font_.setFamily(font->family());
    font_.setPointSize(std::max(font->pointSize(), 8));
  }
  void setBold(     const bool bold     ) { font_.setBold(bold); }
  void setItalics(  const bool italics  ) { font_.setItalic(italics); }
  void setUnderline(const bool underline) { font_.setUnderline(underline); }
  void setStrikeOut(const bool strikeOut) { font_.setStrikeOut(strikeOut); }
};
Q_DECLARE_METATYPE(Preference *)

#endif // PREFERENCE_HPP
