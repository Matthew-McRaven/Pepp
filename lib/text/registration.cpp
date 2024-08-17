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
#include "ScintillaEditBase/ScintillaEditBase.h"
#include "editor/blockfinder.hpp"
#include "editor/lineinfomodel.hpp"
#include "editor/object.hpp"
#include "editor/scintillaasmeditbase.hpp"
#include "editor/tabnanny.hpp"

void text::registerTypes(const char *uri) {
  qmlRegisterType<BlockFinder>(uri, 1, 0, "BlockFinder");
  qmlRegisterType<LineInfoModel>(uri, 1, 0, "LineInfoModel");
  qmlRegisterType<TabNanny>(uri, 1, 0, "TabNanny");
  qmlRegisterType<ObjectUtilities>("edu.pepp.text", 1, 0, "ObjectUtilities");
  qmlRegisterType<ScintillaEditBase>("org.scintilla.scintilla", 1, 0, "ScintillaEditBase");
  qmlRegisterType<ScintillaAsmEditBase>("org.scintilla.scintilla", 1, 0, "ScintillaAsmEdit");
}
