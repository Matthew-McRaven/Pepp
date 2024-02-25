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

#include <QObject>
#include "../highlight_globals.hpp"
#include "../style.hpp"
#include "./types.hpp"

// Maybe I could access as properties if I used this... https://doc.qt.io/qt-6/qqmlpropertymap.html
namespace highlight::style {
class HIGHLIGHT_EXPORT Map : public QObject {
  Q_OBJECT

public:
  Map(QObject *parent = nullptr);

  Q_INVOKABLE void clear();
  Q_INVOKABLE ::highlight::Style *getStyle(Types type) const;
  // returns true if style was changed
  Q_INVOKABLE bool setStyle(Types type, ::highlight::Style *newStyle);

signals:
  void styleChanged();

private:
  QMap<Types, ::highlight::Style *> _styles = {};
};
}; // namespace highlight::style
