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

#include "parser.hpp"
#include "macro/parse.hpp"
MacroParseResult::MacroParseResult(QObject *parent) : QObject(parent) {}

MacroParseResult::MacroParseResult(QObject *parent, bool valid, QString name,
                                   quint8 argc)
    : QObject(parent), _valid(valid), _name(name), _argc(argc) {}

bool MacroParseResult::valid() const { return _valid; }

QString MacroParseResult::name() const { return _name; }

quint8 MacroParseResult::argc() const { return _argc; }

MacroParseResult *MacroParser::parse(QString arg) {
  // Parent should be nullptr, as we want to explicitly transfer ownership to
  // caller.
  auto parse = macro::analyze_macro_definition(arg);
  if (!std::get<0>(parse))
    return new MacroParseResult(nullptr);
  else
    return new MacroParseResult(nullptr, true, std::get<1>(parse),
                                std::get<2>(parse));
}
