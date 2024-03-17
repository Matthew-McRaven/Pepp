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
#include <QSharedData>
#include <QExplicitlySharedDataPointer>
#include "frontend_globals.hpp"

class PreferencePrivate : public QSharedData {
public:
  PreferencePrivate() = default;
  virtual ~PreferencePrivate() = default;

  quint32 id_ = 0;
  quint32 parentId_ = 0;
  QString name_{};
  QColor foreground_{Qt::black};
  QColor background_{Qt::white};
  QFont font_;
};


//  Class is implemented as a PIMPL pattern because the structure is passed
//  to QML as a copy. When QML is updated, the copy sees the changes but not
//  the original object. Using PIMPL pattern forces all copies to reference
//  same data structure.
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
  Q_PROPERTY(bool strikeout READ strikeOut WRITE setStrikeOut);

  //Preference(PreferencePrivate* p) : d(p){};
  QExplicitlySharedDataPointer<PreferencePrivate> d;

public:

  Preference() : d(new PreferencePrivate){}
  ~Preference() = default;


  Preference(const quint32 id, const QString name) :
    d(new PreferencePrivate)
  {
    d->id_ = id;
    d->name_ = name;
  }

  Preference(const quint32 id, const QString name, const QRgb foreground,const QRgb background,
             const quint32 parentId = 0, const bool bold = false, const bool italics = false, const bool underline = false,
             const bool strikeout = false) :
  d(new PreferencePrivate)
  {
    d->id_ = id;
    d->name_ = name;
    d->foreground_ = foreground;
    d->background_ = background;
    d->parentId_ = parentId;

    d->font_.setBold(bold);
    d->font_.setItalic(italics);
    d->font_.setUnderline(underline);
    d->font_.setStrikeOut(strikeout);
  }

  Preference(const Preference &) = default;
  Preference &operator=(const Preference &) = default;
  //	Moving OK
  Preference(Preference &&) noexcept = default;
  Preference &operator=(Preference &&) = default;

  size_t size() const { return 11; }

  //  Getter & Setter
  quint32 id() const { return d->id_; }
  QString name() const { return d->name_; }

  quint32 parentId() const { return d->parentId_; }
  QColor foreground() const { return d->foreground_; }
  QColor background() const { return d->background_; }
  QFont font() const { return d->font_; }

  bool bold()      const { return d->font_.bold(); }
  bool italics()   const { return d->font_.italic(); }
  bool underline() const { return d->font_.underline(); }
  bool strikeOut() const { return d->font_.strikeOut(); }

  void setParent(const quint32 parent) { d->parentId_ = parent; }
  void setForeground(const QColor foreground) { d->foreground_ = foreground; }
  void setBackground(const QColor background) { d->background_ = background; }
  void setFont(QFont *font) {
    //  Only using font family and pointsize. If same, font has not changed
    if (d->font_.family() == font->family() &&
        d->font_.pointSize() == font->pointSize())
      return;

    d->font_.setFamily(font->family());
    //  Font must always be 8 points or larger
    d->font_.setPointSize(std::max(font->pointSize(), 8));
  }
  void setBold(     const bool bold     ) { d->font_.setBold(bold); }
  void setItalics(  const bool italics  ) { d->font_.setItalic(italics); }
  void setUnderline(const bool underline) { d->font_.setUnderline(underline); }
  void setStrikeOut(const bool strikeOut) { d->font_.setStrikeOut(strikeOut); }
};
Q_DECLARE_METATYPE(Preference)

#endif // PREFERENCE_HPP
