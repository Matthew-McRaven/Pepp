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
class Registered;

class Registry : public QObject {
  Q_OBJECT
public:
  explicit Registry(QObject *parent = nullptr);
  bool contains(QString name) const;
  // Returns nullptr if not found.
  const Registered *findMacro(QString name) const;
  // FIXME: Replace with an iterator so as not to force additional memory
  // allocations.
  QList<const Registered *> findMacrosByType(types::Type type) const;
  void clear();
  // Ownership of macro is always transfered to this.
  // Returns nullptr if the macro already exists in the registry.
  // Returned pointer is non-owning
  QSharedPointer<const Registered> registerMacro(types::Type type, QSharedPointer<Parsed> macro);
  void removeMacro(QString name);

signals:
  //! Emitted when a macro is successfully registered or removed.
  void macrosChanged();
  //! Emitted when clear() is called.
  void cleared();

private:
  //
  QMap<QString, QSharedPointer<Registered>> _macros;
};
} // namespace macro
