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

#include "registration.hpp"
#include "editor/blockfinder.hpp"
#include "editor/lineinfomodel.hpp"
#include "editor/object.hpp"
#include "editor/tabnanny.hpp"
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
  qmlRegisterType<BlockFinder>(uri, 1, 0, "BlockFinder");
  qmlRegisterType<LineInfoModel>(uri, 1, 0, "LineInfoModel");
  qmlRegisterType<TabNanny>(uri, 1, 0, "TabNanny");
  qmlRegisterType<ObjectUtilities>("edu.pepp.text", 1, 0, "ObjectUtilities");
}
