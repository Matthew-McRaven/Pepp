/*
 * Copyright (c) 2023-2026 J. Stanley Warford, Matthew McRaven
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

#include "macro.hpp"
#include <iostream>
#include "../../shared.hpp"
#include "toolchain/helpers/asmb.hpp"
#include "toolchain/helpers/assemblerregistry.hpp"

GetMacroTask::GetMacroTask(int ed, std::string name, QObject *parent) : Task(parent), ed(ed), name(name) {}

void GetMacroTask::run() {
  auto books = helpers::builtins_registry(false);
  if (auto book = helpers::book(ed, &*books); book == nullptr) return emit finished(1);
  else if (auto macro = book->find_macro(name); macro == nullptr) return emit finished(1);
  else std::cout << macro->body << std::endl;
  return emit finished(0);
}
