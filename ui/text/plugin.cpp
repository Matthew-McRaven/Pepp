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

#include "plugin.hpp"
#include "highlight/qml_highlighter.hpp"
#include "highlight/style.hpp"
#include "highlight/style/defaults.hpp"
#include "highlight/style/map.hpp"

void text::registerTypes(const char *uri) {
  qmlRegisterType<highlight::QMLHighlighter>(uri, 1, 0, "Highlighter");
  qmlRegisterType<highlight::Style>(uri, 1, 0, "Style");
  qmlRegisterType<highlight::style::Map>(uri, 1, 0, "StyleMap");
  qmlRegisterSingletonInstance<highlight::style::Defaults>(uri, 1, 0, "DefaultStyles",
                                                           new highlight::style::Defaults());
}
