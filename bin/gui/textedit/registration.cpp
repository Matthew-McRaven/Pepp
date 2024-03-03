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
#include <QQuickTextDocument>
#include <QTextBlock>
#include "help/builtins/registry.hpp"
#include "highlight/qml_highlighter.hpp"
#include "highlight/style.hpp"
#include "highlight/style/defaults.hpp"
#include "highlight/style/map.hpp"
#include "lineinfomodel.hpp"

namespace textedit {
void registerTypes(QQmlApplicationEngine &engine) {
  // TODO: Missing translations
  qmlRegisterType<highlight::QMLHighlighter>("edu.pepp", 1, 0, "Highlighter");
  qmlRegisterType<highlight::Style>("edu.pepp", 1, 0, "Style");
  qmlRegisterType<highlight::style::Map>("edu.pepp", 1, 0, "StyleMap");
  qmlRegisterType<BlockFinder>("edu.pepp", 1, 0, "BlockFinder");
  qmlRegisterSingletonInstance<highlight::style::Defaults>("edu.pepp", 1, 0, "DefaultStyles",
                                                           new highlight::style::Defaults());
  qmlRegisterType<LineInfoModel>("edu.pepp", 1, 0, "LineInfoModel");
}
} // namespace textedit

BlockFinder::BlockFinder(QObject *parent) : QObject(parent) {}
int BlockFinder::find_pos(int pos) {
  if (_doc == nullptr)
    return -1;
  return _doc->findBlock(pos).blockNumber();
}

void BlockFinder::set_document(QQuickTextDocument *doc) { this->_doc = doc->textDocument(); }
