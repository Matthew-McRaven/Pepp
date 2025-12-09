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
#include <tuple>

namespace macro {
// Analyze a macro's text body, and attempt to extract header information.
// Tuple returns 1) is the header well formed, 2) what is the macro's name,
// 3) how many arguments does the macro require?
std::tuple<bool, QString, quint8> analyze_macro_definition(QString macro_text);
}; // End namespace macro
