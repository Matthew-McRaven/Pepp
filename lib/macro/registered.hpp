/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
 *
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

#include "./types.hpp"

namespace macro {
class Parsed;
class Registered : public QObject {
  Q_OBJECT
  Q_PROPERTY(const Parsed *contents READ contentsPtr CONSTANT)
  Q_PROPERTY(types::Type type READ type CONSTANT)
public:
  // Takes ownership of contents and changes its parent to this
  Registered(types::Type type, QSharedPointer<const Parsed> contents);
  // Needed to access from QML.
  const Parsed *contentsPtr() const;
  QSharedPointer<const Parsed> contents() const;
  types::Type type() const;

private:
  QSharedPointer<const Parsed> _contents;
  types::Type _type;
};
} // namespace macro
