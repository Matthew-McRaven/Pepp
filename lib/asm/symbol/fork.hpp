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
#include <QtCore>

namespace symbol {
class Table;
class Entry;
class ForkMap {
public:
  void addMapping(const Table *from, QSharedPointer<Table> to);
  void addMapping(const Entry *from, QSharedPointer<Entry> to);
  QSharedPointer<Table> map(const Table *from);
  QSharedPointer<Entry> map(const Entry *from);

private:
  QMap<const Table *, QSharedPointer<Table>> _tableMap;
  QMap<const Entry *, QSharedPointer<Entry>> _symbolMap;
};

QSharedPointer<ForkMap> fork(QSharedPointer<const symbol::Table> from);
} // namespace symbol
