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
#pragma once

#include <QColor>
#include <QExplicitlySharedDataPointer>
#include <QFont>
#include <QObject>
#include "preference_p.hpp"

//  Class is implemented as a PIMPL pattern because the structure is passed
//  to QML as a copy. When QML is updated, the copy sees the changes but not
//  the original object. Using PIMPL pattern forces all copies to reference
//  same data structure.
class Preference : public QObject {
  Q_OBJECT
  // Q_PROPERTY(int id       READ id   CONSTANT)
  Q_PROPERTY(Themes::Roles id READ id CONSTANT)
  Q_PROPERTY(QString name READ name CONSTANT)
  Q_PROPERTY(QColor foreground READ foreground WRITE setForeground NOTIFY preferenceChanged)
  Q_PROPERTY(QColor background READ background WRITE setBackground NOTIFY preferenceChanged)
  Q_PROPERTY(QFont font READ font CONSTANT)
  Q_PROPERTY(bool bold READ bold WRITE setBold NOTIFY preferenceChanged);
  Q_PROPERTY(bool italics READ italics WRITE setItalics NOTIFY preferenceChanged);
  Q_PROPERTY(bool underline READ underline WRITE setUnderline NOTIFY preferenceChanged);
  Q_PROPERTY(bool strikeout READ strikeOut WRITE setStrikeOut NOTIFY preferenceChanged);

  // Preference(PreferencePrivate* p) : d(p){};
  QExplicitlySharedDataPointer<PreferencePrivate> d;

public:
  explicit Preference(QObject *parent = nullptr);
  virtual ~Preference() = default;

  Preference(QObject *parent, const Themes::Roles id, const QString name);
  Preference(QObject *parent, const Themes::Roles id, const QString name, const QRgb foreground, const QRgb background,
             const quint32 parentId = 0, const bool bold = false, const bool italics = false,
             const bool underline = false, const bool strikeout = false);

  //  Cannot move or copy QObject
  Preference(const Preference &) = delete;
  Preference &operator=(const Preference &) = delete;
  Preference(Preference &&) noexcept = delete;
  Preference &operator=(Preference &&) = delete;

  size_t size() const;

  //  Getter & Setter
  // int id() const;
  Themes::Roles id() const;
  QString name() const;

  int parentId() const;
  QColor foreground() const;
  QColor background() const;
  QFont font() const;

  bool bold() const;
  bool italics() const;
  bool underline() const;
  bool strikeOut() const;

  void setParent(const quint32 parent);
  void setForeground(const QColor foreground);
  void setBackground(const QColor background);
  void setFont(QFont *font);
  void setBold(const bool bold);
  void setItalics(const bool italics);
  void setUnderline(const bool underline);
  void setStrikeOut(const bool strikeOut);

  QJsonObject toJson() const;
  static bool fromJson(const QJsonObject &json, Preference &pref);

signals:
  void preferenceChanged();

private:
  void setId(const Themes::Roles id);
  void setName(const QString name);
};
