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

namespace highlight::style {
enum Types {
  // Shared between C/C++/Assembly
  Comment = -1,
  Quoted = -2,
  Warning = -3,
  Error = -4,
  // Assembly-only types
  Symbol = 0,
  Mnemonic,
  Dot,
  // C/C++-only types
  FunctionDec,
  Typename,
  Keyword,
  OtherKeyword,
  Class,
};

// used to expose our style types into a QML singleton.
class HIGHLIGHT_EXPORT QMLTypes : public QObject {
  Q_OBJECT
public:
  QMLTypes();
  using enum Types;
};
} // namespace highlight::style
