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

#include "./macro.hpp"

macro::Parsed::Parsed(QString name, quint8 argCount, QString body, QString architecture, QString family, bool hidden)
    : QObject(nullptr), _name(name), _body(body), _architecture(architecture), _family(family), _argCount(argCount),
      _hidden(hidden) {}

QString macro::Parsed::name() const { return _name; }

QString macro::Parsed::body() const { return _body; }

quint8 macro::Parsed::argCount() const { return _argCount; }

QString macro::Parsed::architecture() const { return _architecture; }

QString macro::Parsed::family() const { return _family; }

bool macro::Parsed::hidden() const { return _hidden; }
