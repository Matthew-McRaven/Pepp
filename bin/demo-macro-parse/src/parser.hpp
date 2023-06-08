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
struct MacroParseResult : public QObject {
  Q_OBJECT;
  Q_PROPERTY(bool valid READ valid CONSTANT)
  Q_PROPERTY(QString name READ name CONSTANT)
  Q_PROPERTY(quint8 argc READ argc CONSTANT)

public:
  MacroParseResult(QObject *parent);
  MacroParseResult(QObject *parent, bool valid, QString name, quint8 argc);
  bool valid() const;
  QString name() const;
  quint8 argc() const;
  bool _valid = false;
  QString _name = "";
  quint8 _argc = 0;
};

class MacroParser : public QObject {
  Q_OBJECT
public:
  // Ownership of parse result is transfered to caller.
  Q_INVOKABLE MacroParseResult *parse(QString arg);
};
