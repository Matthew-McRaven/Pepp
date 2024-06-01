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
#include <qqml.h>
#include "./constants.hpp"
#include "commands.hpp"
#include "opcodemodel.hpp"
#include "strings.hpp"
#include "sequenceconverter.hpp"

void utils::registerTypes(const char *uri) {
  qmlRegisterUncreatableType<utils::AbstractionHelper>(uri, 1, 0, "Abstraction", error_only_enums);
  qmlRegisterUncreatableType<utils::ArchitectureHelper>(uri, 1, 0, "Architecture", error_only_enums);
  qmlRegisterType<OpcodeModel>(uri, 1, 0, "OpcodeModel");
  qmlRegisterUncreatableType<utils::WhichPaneHelper>(uri, 1, 0, "Panes", error_only_enums);
  qmlRegisterUncreatableType<utils::CommandHelper>(uri, 1, 0, "Commands", error_only_enums);
  qmlRegisterSingletonType<utils::SequenceConverter>(uri, 1, 0, "SequenceConverter", [](auto*, auto*){
    return new utils::SequenceConverter();
  });
}
