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

#ifndef PREFERENCE_H_HPP
#define PREFERENCE_H_HPP

#include <QColor>
#include <QFont>
#include <QObject>
#include <QSharedData>

class PreferencePrivate : public QSharedData {
public:
  PreferencePrivate() = default;
  virtual ~PreferencePrivate() = default;

  //  Cannot move or copy QObject
  PreferencePrivate(const PreferencePrivate &) = delete;
  PreferencePrivate &operator=(const PreferencePrivate &) = delete;
  PreferencePrivate(PreferencePrivate &&) noexcept = delete;
  PreferencePrivate &operator=(PreferencePrivate &&) = delete;

  quint32 id_ = 0;
  quint32 parentId_ = 0;
  QString name_{};
  QColor  foreground_{Qt::black};
  QColor  background_{Qt::white};
  QFont   font_;
};

#endif // PREFERENCE_H_HPP
